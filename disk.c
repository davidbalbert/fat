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
