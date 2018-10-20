#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "fat.h"
#include "mbr.h"

typedef u64 Lba;
typedef u64 LbaSize;

// x86 specific

u16
le2cpu16(u16 n)
{
    return n;
}

u32
le2cpu32(u32 n)
{
    return n;
}

typedef struct Drive Drive;
u8 *drive_read(Drive *d, Lba lba, LbaSize count);

typedef struct Partition {
    Drive *drive;
    Lba offset;
    LbaSize nsects;
} Partition;

u8 *
part_read(Partition *p, Lba lba, LbaSize count)
{
    return drive_read(p->drive, lba+p->offset, count);
}

typedef struct Drive {
    u8 *bytes;
    u16 sect_size;
    LbaSize nsects;
} Drive;

u8 *
drive_read(Drive *d, Lba lba, LbaSize count)
{
    if (lba + count > d->nsects) {
        fprintf(stderr, "Can't read past end of drive (lba = %llu, count = %llu, but disk only has %llu sectors)\n", lba, count, d->nsects);
        exit(1);
    }
    return d->bytes + lba*d->sect_size;
}

Mbr *
drive_mbr(Drive *d)
{
    return (Mbr *)d->bytes;
}

int
drive_part_count(Drive *d)
{
    return 4;
}

typedef struct Mbr Mbr;

Partition
drive_get_part(Drive *d, int partnum)
{
    Mbr *mbr = (Mbr *)d->bytes;
    Partition p;

    if (partnum >= MBR_PARTCOUNT) {
        fprintf(stderr, "Can't read partition number %d, max partition is %d\n", partnum, drive_part_count(d));
    }

    MbrPartitionEntry *pe = &mbr->partition_table[partnum];

    p.drive = d;
    p.offset = mbr_pe_lba_start(pe);
    p.nsects = mbr_pe_lba_size(pe);

    return p;
}

int
mbr_bootable(Mbr *mbr)
{
    return mbr->sig[0] == 0x55 && mbr->sig[1] == 0xAA;
}

int mbr_pe_active(MbrPartitionEntry *pe)
{
    return pe->boot == 0x80;
}

int
mbr_pe_c(u8 chs[3])
{
    return ((chs[1] & 0xC0) << 2) + chs[2];
}

int
mbr_pe_h(u8 chs[3])
{
    return chs[0];
}

int
mbr_pe_s(u8 chs[3])
{
    return chs[1] & 0x3F;
}

u32
mbr_pe_lba_start(MbrPartitionEntry *pe)
{
    return le2cpu32(pe->lba_start);
}

u32
mbr_pe_lba_size(MbrPartitionEntry *pe)
{
    return le2cpu32(pe->lba_size);
}

u32
fat_nsects(FatBpb *bpb)
{
    return bpb->nsects_16 ? le2cpu16(bpb->nsects_16) : le2cpu32(bpb->nsects_32);
}

u32
fat_fatsz(FatBpb *bpb)
{
    return bpb->fatsz_16 ? le2cpu16(bpb->fatsz_16) : le2cpu32(((Fat32Bpb *)bpb)->fatsz_32);
}

u16
fat_bytes_per_sect(FatBpb *bpb)
{
    return le2cpu16(bpb->bytes_per_sect);
}

u16
fat_nroot_entries(FatBpb *bpb)
{
    return le2cpu16(bpb->nroot_entries);
}

u32
fat_nreserved_sects(FatBpb *bpb)
{
    return le2cpu16(bpb->nreserved_sects);
}

u8
fat_sects_per_cluster(FatBpb *bpb)
{
    return bpb->sects_per_cluster;
}

u32
fat_nclusters(FatBpb *bpb)
{
    u32 nsects = fat_nsects(bpb);
    u32 fatsz = fat_fatsz(bpb);

    u16 bps = fat_bytes_per_sect(bpb);
    u32 nroot_sects = ((fat_nroot_entries(bpb)*32) + (bps - 1)) / bps;
    u32 nreserved = fat_nreserved_sects(bpb);

    u32 ndata_sects = nsects - (nreserved + bpb->nfats*fatsz + nroot_sects);

    return ndata_sects / fat_sects_per_cluster(bpb);
}

FatType
fat_type(FatBpb *bpb)
{
    u32 nclusters = fat_nclusters(bpb);

    if (nclusters < 4085) {
        return FAT12;
    } else if (nclusters < 65525) {
        return FAT16;
    } else {
        return FAT32;
    }
}

char *
fat_type_str(FatType v)
{
    switch (v) {
    case FAT12:
        return "FAT12";
    case FAT16:
        return "FAT16";
    case FAT32:
        return "FAT32";
    }
}

void
diskinfo(char *path)
{
    int fd, ret;
    u8 *p;
    struct stat st;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    ret = fstat(fd, &st);
    if (ret == -1) {
        perror("fstat");
        exit(1);
    }

    p = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    Drive d = {
        .bytes = p,
        .sect_size = 512,
        .nsects = st.st_size/512,
    };

    printf("disk: %s\n", path);
    printf("bootable: %s\n", mbr_bootable(drive_mbr(&d)) ? "yes" : "no");

    for (int i = 0; i < MBR_PARTCOUNT; i++) {
        MbrPartitionEntry *pe = &drive_mbr(&d)->partition_table[i];

        printf("%s%d: (%d, %d, %d) - (%d, %d, %d), [lba: %d, %d]\n",
            mbr_pe_active(pe) ? "*" : " ",
            i + 1,
            mbr_pe_c(pe->chs_start),
            mbr_pe_h(pe->chs_start),
            mbr_pe_s(pe->chs_start),
            mbr_pe_c(pe->chs_end),
            mbr_pe_h(pe->chs_end),
            mbr_pe_s(pe->chs_end),
            mbr_pe_lba_start(pe),
            mbr_pe_lba_size(pe));
    }

    Partition part = drive_get_part(&d, 0);
    FatVbr *vbr = (FatVbr *)part_read(&part, 0, 1);
    printf("\nPartition 1: %s\n", fat_type_str(fat_type(&vbr->bpb)));


    ret = munmap(p, st.st_size);
    if (ret == -1) {
        perror("munmap");
        exit(1);
    }

    ret = close(fd);
    if (ret == -1) {
        perror("close");
        exit(1);
    }
}

char *progname;
void usage(void);

int
main(int argc, char *argv[])
{
    progname = argv[0];

    if (argc < 2) {
        usage();
    }

    diskinfo(argv[1]);

    return 0;
}

void
usage(void)
{
    fprintf(stderr, "usage: %s file\n", progname);
    exit(1);
}
