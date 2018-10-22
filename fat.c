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
fat_dirent_read_short_name(FatDirEnt *ent, char *buf)
{
    int i;
    char *p = buf;

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

// returns 1 if the string is finished, returns 0 otherwise.
int
read_longent_codepoints(char **p, u16 *codepoints, int count)
{
    int i;
    u16 codepoint;

    for (i = 0; i < count; i++) {
        codepoint = le2cpu16(codepoints[i]);

        if (codepoint == 0) {
            **p = '\0';
            *p += 1;
            return 1;
        } else if (codepoint > 0xFF) {
            **p = '?';
        } else {
            **p = codepoint & 0xFF;
        }

        *p += 1;
    }

    return 0;
}

// TODO: actually handle characters beyond ASCII
static char *
fat_dirent_read_long_name(FatLongDirEnt *lent, char *buf)
{
    int i, done;
    char *p = buf;

    if (!(lent->order & FAT_LAST_LONG_ENTRY)) {
        printf("fat_dirent_read_long_name: lent isn't the last entry\n");
        exit(1);
    }

    int nlents = lent->order & ~FAT_LAST_LONG_ENTRY;

    lent += nlents-1;

    for (i = 0; i < nlents; i++) {
        done = read_longent_codepoints(&p, lent->name1, 5);
        if (done) { break; }

        done = read_longent_codepoints(&p, lent->name2, 6);
        if (done) { break; }

        done = read_longent_codepoints(&p, lent->name3, 2);
        if (done) { break; }

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
fat_dirent_read_name(FatDirEnt *ent, char *buf)
{
    if (fat_dirent_is_long(ent)) {
        return fat_dirent_read_long_name((FatLongDirEnt *)ent, buf);
    } else {
        return fat_dirent_read_short_name(ent, buf);
    }
}

FatDirEnt *
fat_dirent_next(FatDirEnt *ent)
{
    if (!fat_dirent_is_long(ent)) {
        return ent + 1;
    }

    FatLongDirEnt *lent = (FatLongDirEnt *)ent;
    int nlents = lent->order & ~FAT_LAST_LONG_ENTRY;

    return ent + nlents + 1; // + 1 to skip our short entry as well
}

FatDirEnt *
fat_get_short_ent(FatDirEnt *ent)
{
    if (!fat_dirent_is_long(ent)) {
        return ent;
    }

    FatLongDirEnt *lent = (FatLongDirEnt *)ent;
    int nlents = lent->order & ~FAT_LAST_LONG_ENTRY;

    return ent + nlents;
}

u32
fat_dirent_filesz(FatDirEnt *ent)
{
    ent = fat_get_short_ent(ent);

    return le2cpu32(ent->filesz);
}

int
fat_dirent_is_long(FatDirEnt *ent)
{
    return (ent->attr & FAT_ATTR_LONG_NAME_MASK) == FAT_ATTR_LONG_NAME;
}

int
fat_dirent_is_dir(FatDirEnt *ent)
{
    ent = fat_get_short_ent(ent);

    return ent->attr & FAT_ATTR_DIRECTORY;
}

FatDirEnt *
fat_root_dir(FatFS *fs)
{
    FatBpb *bpb = fat_get_bpb(fs);

    Lba first_root_sect = fat_nreserved_sects(bpb) + fat_nfats(bpb)*fat_fatsz(bpb);
    LbaCount nroot_sects = fat_nroot_sects(bpb);

    return disk_read(fs->disk, first_root_sect, nroot_sects);
}
