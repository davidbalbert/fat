#include "u.h"

#include "disk.h"

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
