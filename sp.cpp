#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

extern u8 DRAM[8*1024*1024];
extern u8* ROM;
extern u32 rom_size;

u32 sp_regs[13] = {0};

void sp_reg_write32(u32 addr, u32 val)
{
	addr &= 0x3F;
	printf("SP Write: %x to %x\n", val, addr);
	addr >>= 2;

	if( addr > 12 ) return;

	sp_regs[addr] = val;

	return;
}

u32 sp_reg_read32(u32 addr)
{
	addr &= 0x3F;
	addr >>= 2;

	if( addr == 4 ) 
	{
		return 3;
	}

	return sp_regs[addr];
}

void sp_update(int cpu_cycles)
{

	return;
}



