#include "u.h"

#include "disk.h"
#include "part.h"

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
