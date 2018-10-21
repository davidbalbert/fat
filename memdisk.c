#include <stdio.h>
#include <stdlib.h>

#include "u.h"

#include "disk.h"
#include "memdisk.h"

void *
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

MemDisk *
memdisk_init(MemDisk *md, u8 *bytes, u16 sect_size, LbaCount nsects)
{
    disk_init(&md->base, &memdisk_ops);
    md->bytes = bytes;
    md->sect_size = sect_size;
    md->nsects = nsects;

    return md;
}
