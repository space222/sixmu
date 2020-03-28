#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

extern u8 DRAM[8*1024*1024];
extern u8* ROM;
extern u32 rom_size;
extern u32 mi_regs[4];
extern bool UsingInterpreter;
void invalidate_page(u32);

u32 pi_regs[13] = {0};

void pi_reg_write32(u32 addr, u32 val)
{
	addr &= 0x3F;
	printf("PI Write: %x to %x\n", val, addr);
	addr >>= 2;

	if( addr > 12 ) return;

	if( addr == 4 )
	{
		if( val & 2 )
		{
			mi_regs[2] &= ~BIT(4);
		}
		return;
	}

	pi_regs[addr] = val;

	if( addr == 3 )
	{
		u32 start_addr = (pi_regs[0]&0x7FFFF8);
		u8* start = DRAM + start_addr;
		u32 cartaddr = (pi_regs[1]&0xFFFFFFF);
		val++;
		int length = (cartaddr + val) > rom_size ? rom_size - cartaddr : val;
		pi_regs[3] = pi_regs[4] = 0;
		memcpy(start, ROM+cartaddr, length); // yes I know cartaddr could still be beyond ROM
		mi_regs[2] |= BIT(4);

		if( !UsingInterpreter ) 
		{
			for(int i = start_addr; i <= (start_addr+length); i += 0x1000)
			{
				invalidate_page(i>>12);
			}
		}
		return;
	}

	pi_regs[addr] &= 0xff;
	
	return;
}

void pi_update(int cpu_cycles)
{

	return;
}

u32 pi_reg_read32(u32 addr)
{
	addr &= 0x3F;
	addr >>= 2;

	if( addr > 12 ) return 0;

	return pi_regs[addr];
}




