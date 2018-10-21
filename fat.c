#include <stdio.h>
#include <stdlib.h>

#include "u.h"

#include "disk.h"
#include "fat.h"
#include "x86.h"

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

u8
fat_nfats(FatBpb *bpb)
{
    return bpb->nfats;
}

u32
fat_nroot_sects(FatBpb *bpb)
{
    u16 bps = fat_bytes_per_sect(bpb);
    return ((fat_nroot_entries(bpb)*32) + (bps - 1)) / bps;
}

u32
fat_nclusters(FatBpb *bpb)
{
    u32 nsects = fat_nsects(bpb);
    u32 fatsz = fat_fatsz(bpb);

    u32 nroot_sects = fat_nroot_sects(bpb);
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

FatFS *
fat_init(FatFS *fs, Disk *d)
{
    fs->disk = d;

    FatBpb *bpb = disk_read(d, 0, 1);
    fs->type = fat_type(bpb);

    return fs;
}

FatBpb *
fat_get_bpb(FatFS *fs)
{
    FatVbr *vbr = disk_read(fs->disk, 0, 1);
    return &vbr->bpb;
}

static char *
fat_dirent_read_short_name(FatDirEnt *ent, char *buf, FatDirEnt **short_ent)
{
    int i;
    char *p = buf;

    *short_ent = ent;

    for (i = 0; i < 8; i++) {
        if (i == 0 && ent->fname[i] == 0x05) {
            *p = 0xE5;
        } else if (ent->fname[i] == 0x20) {
            break;
        } else {
            *p = ent->fname[i];
        }

        p++;
    }

    if (ent->ext[0] != 0x20) {
        *p = '.';
        p++;
    }

    for (i = 0; i < 3; i++) {
        if (ent->ext[i] == 0x20) {
            break;
        }

        *p = ent->ext[i];
        p++;
    }

    *p = '\0';

    return buf;
}

char *
read_longent_codepoints(char *p, u16 *codepoints)
{
    return (void *)0;
}

// TODO: actually handle characters beyond ASCII
static char *
fat_dirent_read_long_name(FatLongDirEnt *lent, char *buf, FatDirEnt **short_ent)
{
    int i, j;
    u16 codepoint;
    char *p = buf;

    if (!(lent->order & FAT_LAST_LONG_ENTRY)) {
        printf("fat_dirent_read_long_name: lent isn't the last entry\n");
        exit(1);
    }

    int nlents = lent->order & ~FAT_LAST_LONG_ENTRY;

    lent += nlents-1;
    *short_ent += nlents;

    for (i = 0; i < nlents; i++) {
        for (j = 0; j < 5; j++) {
            codepoint = le2cpu16(lent->name1[j]);

            if (codepoint == 0) {
                *p = '\0';
                p++;
                break;
            } else if (codepoint > 0xFF) {
                *p = '?';
            } else {
                *p = codepoint & 0xFF;
            }

            p++;
        }

        if (*(p-1) == '\0') {
            break;
        }

        for (j = 0; j < 6; j++) {
            codepoint = le2cpu16(lent->name2[j]);

            if (codepoint == 0) {
                *p = '\0';
                p++;
                break;
            } else if (codepoint > 0xFF) {
                *p = '?';
            } else {
                *p = codepoint & 0xFF;
            }

            p++;
        }

        if (*(p-1) == '\0') {
            break;
        }

        for (j = 0; j < 2; j++) {
            codepoint = le2cpu16(lent->name3[j]);

            if (codepoint == 0) {
                *p = '\0';
                p++;
                break;
            } else if (codepoint > 0xFF) {
                *p = '?';
            } else {
                *p = codepoint & 0xFF;
            }

            p++;
        }

        if (*(p-1) == '\0') {
            break;
        }

        lent--;
    }

    // handle the case where the filename ends
    // exactly at the end of the lent.
    if (*p != '\0') {
        *p = '\0';
    }

    return buf;
}

// buf must be FAT_NAME_BUF_SIZE;
char *
fat_dirent_read_name(FatDirEnt *ent, char *buf, FatDirEnt **short_ent)
{
    if (fat_dirent_is_long_name(ent)) {
        return fat_dirent_read_long_name((FatLongDirEnt *)ent, buf, short_ent);
    } else {
        return fat_dirent_read_short_name(ent, buf, short_ent);
    }
}

int
fat_dirent_is_long_name(FatDirEnt *ent)
{
    return (ent->attr & FAT_ATTR_LONG_NAME_MASK) == FAT_ATTR_LONG_NAME;
}

FatDirEnt *
fat_root_dir(FatFS *fs)
{
    FatBpb *bpb = fat_get_bpb(fs);

    Lba first_root_sect = fat_nreserved_sects(bpb) + fat_nfats(bpb)*fat_fatsz(bpb);
    LbaCount nroot_sects = fat_nroot_sects(bpb);

    return disk_read(fs->disk, first_root_sect, nroot_sects);
}
