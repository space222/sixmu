#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

u32 si_regs[7] = {0};

void si_reg_write32(u32 addr, u32 val)
{
	addr &= 0x1F;
	printf("SI: Write %x to %x\n", val, addr);
	addr >>= 2;

	if( addr > 6 ) return;

	return;
}

u32 si_reg_read32(u32 addr)
{
	addr &= 0x1F;
	printf("SI: Read @%x\n", addr);
	addr >>= 2;

	if( addr > 6 ) return 0;

	return si_regs[addr];
}

bool si_update(int cpu_cycles)
{
	return true;
}



