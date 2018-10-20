typedef u64 Lba;
typedef u64 LbaCount;

typedef struct DiskOperations DiskOperations;
typedef struct Disk {
    DiskOperations *ops;
} Disk;

typedef struct DiskOperations {
    u8 *(*read)(Disk *self, Lba lba, LbaCount count);
} DiskOperations;


Disk *disk_init(Disk *d, DiskOperations *ops);
u8 *disk_read(Disk *d, Lba lba, LbaCount count);
