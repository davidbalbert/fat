#include "u.h"

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
