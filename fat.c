#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint32_t u32;

#define MBR_PARTCOUNT 4

typedef struct __attribute__((packed)) MbrPartitionEntry {
  u8 boot;
  u8 chs_start[3];
  u8 type;
  u8 chs_end[3];
  u32 lba_start;
  u32 lba_size;
} MbrPartitionEntry;

typedef struct __attribute__((packed)) Mbr {
    u8 bootstrap[436];
    u8 diskid[10];
    MbrPartitionEntry partition_table[4];
    u8 sig[2];
} Mbr;

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

typedef enum FatVersion {
    FAT12,
    FAT16,
    FAT32,
} FatVersion;

char *progname;
void usage(void);

FatVersion
fat_version(char *bytes)
{
    return FAT32;
}

char *
fat_version_str(FatVersion v)
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
    char *p;
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

    Mbr *mbr = (Mbr *)p;

    printf("disk: %s\n", path);
    printf("bootable: %s\n", mbr_bootable(mbr) ? "yes" : "no");

    for (int i = 0; i < MBR_PARTCOUNT; i++) {
        MbrPartitionEntry *pe = &mbr->partition_table[i];

        printf("partition %d:\n", i);
        printf("\tactive: %s\n", mbr_pe_active(pe) ? "yes" : "no");
        printf("\tchs start: %d, %d, %d\n",
            mbr_pe_c(pe->chs_start),
            mbr_pe_h(pe->chs_start),
            mbr_pe_s(pe->chs_start));
        printf("\tchs end: %d, %d, %d\n",
            mbr_pe_c(pe->chs_end),
            mbr_pe_h(pe->chs_end),
            mbr_pe_s(pe->chs_end));
        printf("\tlba_start: %u\n", pe->lba_start);
        printf("\tlba_size: %u\n", pe->lba_size);
    }


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
