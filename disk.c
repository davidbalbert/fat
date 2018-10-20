#include <stdio.h>
#include <stdlib.h>

#include "u.h"

#include "disk.h"
#include "mbr.h"

Disk *
disk_init(Disk *d, DiskOperations *ops)
{
    d->ops = ops;

    return d;
}

u8 *
disk_read(Disk *d, Lba lba, LbaCount count)
{
    return d->ops->read(d, lba, count);
}


// returns a struct, not a pointer. Currently doing all allocation on the stack.
// Bad idea, right?
Partition
disk_get_part(Disk *d, int partnum)
{
    Partition part;

    Mbr *mbr = (Mbr *)disk_read(d, 0, 1);

    if (partnum >= MBR_PARTCOUNT) {
        // todo error handling
        fprintf(stderr, "Can't read partition number %d, highest partition is %d\n", partnum, MBR_PARTCOUNT-1);
        exit(1);
    }

    MbrPartitionEntry *pe = &mbr->partition_table[partnum];

    return *part_init(&part, d, mbr_pe_lba_start(pe), mbr_pe_lba_size(pe));
}

u8 *
part_read(Disk *self, Lba lba, LbaCount count)
{
    Partition *part = (Partition *)self;

    return disk_read(part->owner, lba+part->offset, count);
}

DiskOperations partition_ops = {
    .read = part_read,
};

Partition *
part_init(Partition *part, Disk *owner, Lba offset, LbaCount nsects)
{
    disk_init(&part->base, &partition_ops);
    part->owner = owner;
    part->offset = offset;
    part->nsects = nsects;

    return part;
}
