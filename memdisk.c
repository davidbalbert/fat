#include <stdio.h>
#include <stdlib.h>

#include "u.h"

#include "disk.h"
#include "part.h"
#include "memdisk.h"
#include "mbr.h"

u8 *
memdisk_read(Disk *d, Lba lba, LbaCount count)
{
    MemDisk *md = (MemDisk *)d;

    if (lba + count > md->nsects) {
        // todo, error handling?
        fprintf(stderr, "Can't read past end of drive (lba = %lu, count = %lu, but disk only has %lu sectors)\n", lba, count, md->nsects);
        exit(1);
    }
    return md->bytes + lba*md->sect_size;
}

DiskOperations memdisk_ops = {
    .read = memdisk_read,
};

// returns a struct, not a pointer. Currently doing all allocation on the stack.
// Bad idea, right?
Partition
memdisk_get_part(MemDisk *md, int partnum)
{
    Partition part;

    Mbr *mbr = (Mbr *)md->bytes;

    if (partnum >= MBR_PARTCOUNT) {
        // todo error handling
        fprintf(stderr, "Can't read partition number %d, highest partition is %d\n", partnum, MBR_PARTCOUNT-1);
        exit(1);
    }

    MbrPartitionEntry *pe = &mbr->partition_table[partnum];

    return *part_init(&part, (Disk *)md, mbr_pe_lba_start(pe), mbr_pe_lba_size(pe));
}

MemDisk *
memdisk_init(MemDisk *md, u8 *bytes, u16 sect_size, LbaCount nsects)
{
    disk_init(&md->base, &memdisk_ops);
    md->bytes = bytes;
    md->sect_size = sect_size;
    md->nsects = nsects;

    return md;
}
