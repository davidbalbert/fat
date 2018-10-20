#include "u.h"

#include "mbr.h"
#include "x86.h"

int
mbr_bootable(Mbr *mbr)
{
    return mbr->sig[0] == 0x55 && mbr->sig[1] == 0xAA;
}

int
mbr_pe_active(MbrPartitionEntry *pe)
{
    return pe->boot == 0x80;
}

int
mbr_pe_c(u8 chs[3])
{
    return ((chs[1] & 0xC0) << 2) + chs[2];
}

int
mbr_pe_h(u8 chs[3])
{
    return chs[0];
}

int
mbr_pe_s(u8 chs[3])
{
    return chs[1] & 0x3F;
}

u32
mbr_pe_lba_start(MbrPartitionEntry *pe)
{
    return le2cpu32(pe->lba_start);
}

u32
mbr_pe_lba_size(MbrPartitionEntry *pe)
{
    return le2cpu32(pe->lba_size);
}
