#define MBR_PARTCOUNT 4

typedef struct __attribute__((packed)) MbrPartitionEntry {
  u8 boot;
  u8 chs_start[3];
  u8 type;
  u8 chs_end[3];
  u32 lba_start;
  u32 lba_size;
} MbrPartitionEntry;

typedef struct __attribute__((packed)) Mbr {
    u8 bootstrap[436];
    u8 diskid[10];
    MbrPartitionEntry partition_table[4];
    u8 sig[2];
} Mbr;

int mbr_bootable(Mbr *mbr);

int mbr_pe_c(u8 chs[3]);
int mbr_pe_h(u8 chs[3]);
int mbr_pe_s(u8 chs[3]);

u32 mbr_pe_lba_start(MbrPartitionEntry *pe);
u32 mbr_pe_lba_size(MbrPartitionEntry *pe);
