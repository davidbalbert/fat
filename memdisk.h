typedef struct MemDisk {
    Disk base;
    u8 *bytes;
    u16 sect_size;
    LbaCount nsects;
} MemDisk;

MemDisk *memdisk_init(MemDisk *md, u8 *bytes, u16 sect_size, LbaCount nsects);
