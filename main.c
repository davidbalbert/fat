#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "u.h"

#include "disk.h"
#include "memdisk.h"
#include "fat.h"
#include "mbr.h"

char *cwd;

char *
month2str(u8 month) {
    switch (month) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "ERR";
    }
}

void
ls(char *cmd, Partition *part)
{
    FatFS fs;
    FatDirEnt *ent;
    fat_init(&fs, (Disk *)part);
    char name[FAT_NAME_BUF_SIZE];

    ent = fat_root_dir(&fs);

    for (ent = fat_root_dir(&fs); ent->fname[0] != 0; ent = fat_dirent_next(ent)) {
        if (ent->fname[0] == 0xE5) {
            continue;
        }

        if (fat_dirent_is_dir(ent)) {
            printf("d");
        } else {
            printf("-");
        }

        if (fat_dirent_readonly(ent)) {
            printf("r-xr-xr-x\t");
        } else {
            printf("rwxrwxrwx\t");
        }

        printf("%uB\t", fat_dirent_filesz(ent));

        u16 mdate = fat_dirent_mdate(ent);
        u16 mtime = fat_dirent_mtime(ent);

        printf("%s %02d %04d %02d:%02d\t",
            month2str(fat_month(mdate)),
            fat_day(mdate),
            fat_year(mdate),
            fat_hours(mtime),
            fat_minutes(mtime));

        printf("%s", fat_dirent_read_name(ent, name));

        if (fat_dirent_is_dir(ent)) {
            printf("/\n");
        } else {
            printf("\n");
        }
    }
}

void
cat(char *cmd, Partition *part)
{
    printf("cat\n");
}

void
cd(char *cmd, Partition *part)
{
    printf("cd\n");
}

void
pwd(char *cmd, Partition *part)
{
    printf("%s\n", cwd);
}

void
help(char *cmd, Partition *part)
{
    printf("Available commands:\n");
    printf("- ls\n");
    printf("- cat\n");
    printf("- cd\n");
    printf("- pwd\n");
    printf("- help\n");
    printf("- exit\n");
}

void
shell(char *path)
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

    MemDisk md;
    memdisk_init(&md, p, 512, st.st_size/512);

    cwd = "/";

    /*
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

    FatVbr *vbr = (FatVbr *)part_read(&part, 0, 1);
    printf("\nPartition 1: %s\n", fat_type_str(fat_type(&vbr->bpb)));
    */

    #define CMD_SIZE 1024
    char cmd[CMD_SIZE];
    char *s;

    Partition part = disk_get_part((Disk *)&md, 0);

    while (1) {
        printf("> ");
        s = fgets(cmd, CMD_SIZE, stdin);

        if (s == NULL) {
            break;
        }

        if (strncmp(cmd, "ls", strlen("ls")) == 0) {
            ls(cmd, &part);
        } else if (strncmp(cmd, "cat", strlen("cat")) == 0) {
            cat(cmd, &part);
        } else if (strncmp(cmd, "cd", strlen("cd")) == 0) {
            cd(cmd, &part);
        } else if (strncmp(cmd, "pwd", strlen("pwd")) == 0) {
            pwd(cmd, &part);
        } else if (strncmp(cmd, "help", strlen("help")) == 0) {
            help(cmd, &part);
        } else if (strncmp(cmd, "exit", strlen("exit")) == 0) {
            break;
        } else {
            printf("Unknown command\n");
        }
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

char *progname;
void usage(void);

int
main(int argc, char *argv[])
{
    progname = argv[0];

    if (argc < 2) {
        usage();
    }

    shell(argv[1]);

    return 0;
}

void
usage(void)
{
    fprintf(stderr, "usage: %s file\n", progname);
    exit(1);
}
