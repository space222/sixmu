#include <cstring>
#include <cstdlib>
#include "types.h"

u32 mi_regs[4] = {0};

void mi_reg_write32(u32 addr, u32 val)
{
	addr &= 0xF;
	addr >>= 2;

	if( addr > 3 ) return;

	if( addr == 0 )
	{
		//bit11 clear DP interupt
		mi_regs[2] &= ~BIT(5);
		return;
	}

	if( addr == 3 )
	{
		if( val&BIT(0) ) { mi_regs[addr] &= ~BIT(0); }
		else if( val&BIT(1) ) { mi_regs[addr] |= BIT(0); }
		if( val&BIT(2) ) { mi_regs[addr] &= ~BIT(1); }
		else if( val&BIT(3) ) { mi_regs[addr] |= BIT(1); }
		if( val&BIT(4) ) { mi_regs[addr] &= ~BIT(2); }
		else if( val&BIT(5) ) { mi_regs[addr] |= BIT(2); }
		if( val&BIT(6) ) { mi_regs[addr] &= ~BIT(3); }
		else if( val&BIT(7) ) { mi_regs[addr] |= BIT(3); }
		if( val&BIT(8) ) { mi_regs[addr] &= ~BIT(4); }
		else if( val&BIT(9) ) { mi_regs[addr] |= BIT(4); }
		if( val&BIT(10) ) { mi_regs[addr] &= ~BIT(5); }
		else if( val&BIT(11) ) { mi_regs[addr] |= BIT(5); }
		mi_regs[addr] &= 0x3F;
		return;
	}
	
	return;
}

u32 mi_reg_read32(u32 addr)
{
	addr &= 0xF;
	addr >>= 2;

	if( addr > 3 ) return 0;

	if( addr == 1 ) 
	{
		return 0x02020102;
	}

	return mi_regs[addr];
}

bool mi_update(int cpu_cycles)
{

	return true;
}



