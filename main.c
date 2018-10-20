#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "u.h"

#include "fat.h"
#include "mbr.h"

typedef u64 Lba;
typedef u64 LbaSize;

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
        fprintf(stderr, "Can't read past end of drive (lba = %lu, count = %lu, but disk only has %lu sectors)\n", lba, count, d->nsects);
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
