typedef struct Partition {
    Disk base;
    Disk *owner;
    Lba offset;
    LbaCount nsects;
} Partition;

Partition *part_init(Partition *part, Disk *owner, Lba offset, LbaCount nsects);
