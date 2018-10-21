typedef enum FatType {
    FAT12,
    FAT16,
    FAT32,
} FatType;

typedef struct __attribute__((packed)) FatBpb {
    u16 bytes_per_sect;
    u8  sects_per_cluster;
    u16 nreserved_sects;
    u8  nfats;
    u16 nroot_entries; // 32 bytes per entry
    u16 nsects_16;
    u8  media_type;
    u16 fatsz_16; // in sectors
    u16 sects_per_track;
    u16 nheads;
    u32 nhidden_sects;
    u32 nsects_32;
} FatBpb;

typedef struct __attribute__((packed)) Fat32Bpb {
    FatBpb base;
    u32 fatsz_32;
    u16 flags;
    u16 version;
    u32 root_cluster; // usually 2. Set this to 2 when formatting.
    u16 fsinfo_sect;
    u16 backup_bootsect; // usually 6
    u8  reserved[12];
} Fat32Bpb;

typedef struct __attribute__((packed)) FatVbr {
    u8 jmp[3];
    char oem_name[8];
    FatBpb bpb;
} FatVbr;

typedef struct __attribute__((packed)) FatDirEnt {
    uchar fname[8];
    uchar ext[3];
    u8   attr;
    u8   reserved;
    u8   ctime_tenths; // 0-199.
    u16  ctime;
    u16  cdate;
    u16  adate; // last accessed (read or write
    u16  first_cluster_hi;
    u16  mtime;
    u16  mdate;
    u16  first_cluster_lo;
    u32  filesz;
} FatDirEnt;

typedef struct __attribute__((packed)) FatLongDirEnt {
    u8 order;
    char name1[10];
    u8 attr;
    u8 type; // should be 0
    u8 checksum;
    char name2[12];
    u16 first_cluster_lo; // must be zero. meaningless.
    char name3[4];
} FatLongDirEnt;

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LONG_NAME (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE)

#define FAT_LAST_LONG_ENTRY 0x40

#define FAT_NAME_BUF_SIZE 256

u32 fat_nsects(FatBpb *bpb);
u32 fat_fatsz(FatBpb *bpb);
u16 fat_bytes_per_sect(FatBpb *bpb);
u16 fat_nroot_entries(FatBpb *bpb);
u32 fat_nreserved_sects(FatBpb *bpb);
u8 fat_sects_per_cluster(FatBpb *bpb);
u32 fat_nclusters(FatBpb *bpb);

FatType fat_type(FatBpb *bpb);
char *fat_type_str(FatType v);


typedef struct FatFS {
    FatType type;
    Disk *disk;
} FatFS;

FatFS *fat_init(FatFS *fs, Disk *disk);
FatDirEnt *fat_root_dir(FatFS *fs);

int fat_dirent_is_long_name(FatDirEnt *ent);
char *fat_dirent_read_name(FatDirEnt *ent, char *buf);
