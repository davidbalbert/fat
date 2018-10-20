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
    char fname[8];
    char ext[3];
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

u32 fat_nsects(FatBpb *bpb);
u32 fat_fatsz(FatBpb *bpb);
u16 fat_bytes_per_sect(FatBpb *bpb);
u16 fat_nroot_entries(FatBpb *bpb);
u32 fat_nreserved_sects(FatBpb *bpb);
u8 fat_sects_per_cluster(FatBpb *bpb);
u32 fat_nclusters(FatBpb *bpb);

FatType fat_type(FatBpb *bpb);
char *fat_type_str(FatType v);

//void fat_init(FatFS *fs, DriveAccessFuncs *funcs, void *arg);
