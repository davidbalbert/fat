typedef u64 Lba;
typedef u64 LbaCount;

typedef struct DiskOperations DiskOperations;
typedef struct Disk {
    DiskOperations *ops;
} Disk;

typedef struct DiskOperations {
    void *(*read)(Disk *self, Lba lba, LbaCount count);
} DiskOperations;


Disk *disk_init(Disk *d, DiskOperations *ops);
void *disk_read(Disk *d, Lba lba, LbaCount count);


typedef struct Partition {
    Disk base;
    Disk *owner;
    Lba offset;
    LbaCount nsects;
} Partition;

Partition *part_init(Partition *part, Disk *owner, Lba offset, LbaCount nsects);
Partition disk_get_part(Disk *d, int partnum);
