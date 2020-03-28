#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

u32 si_regs[7] = {0};
extern u32 mi_regs[4];
extern u8 PIF[0x800];
extern u8 DRAM[8*1024*1024];

void si_do_pif()
{
	printf("PIF: parsing pif data!\n");
	return;
}

void si_reg_write32(u32 addr, u32 val)
{
	addr &= 0x1F;
//	printf("SI: Write %x to %x\n", val, addr);
	addr >>= 2;

//	if( addr > 6 ) return;

	if( addr == 0 )
	{
		si_regs[addr] = val;
		return;
	}

	if( addr == 6 )
	{
		// clears SI interrupt
		mi_regs[2] &= ~BIT(1);
		si_regs[6] &= ~BIT(12);
		return;
	}

	bool interrupt = false;

	if( addr == 1 )
	{
		u32 start = si_regs[0]&0x7fffff;

		for(int i = 0; i < 64 && start+i<0x800000; ++i)
		{
			DRAM[start+i] = PIF[0x7c0 + i];
		}

		interrupt = true;
	} else if( addr == 4 ) {
		u32 start = si_regs[0]&0x7fffff;

		for(int i = 0; i < 64 && start+i<0x800000; ++i)
		{
			PIF[0x7c0 + i] = DRAM[start+i];
		}

		si_do_pif();
		interrupt = true;
	}


	if( interrupt )
	{
		mi_regs[2] |= BIT(1);
		si_regs[6] |= BIT(12);
	}

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



