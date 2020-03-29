#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

extern u8 DRAM[8*1024*1024];
extern u8 DMEM[0x1000];
extern u8 IMEM[0x1000];
extern u8* ROM;
extern u32 rom_size;
extern u32 mi_regs[4];

bool rsp_running = false;

struct rsptask {
	u32	type;
	u32	flags;

	u32	ucode_boot_addr;
	u32	ucode_boot_size;

	u32	ucode_addr;
	u32	ucode_size;

	u32	ucode_data_addr;
	u32	ucode_data_size;

	u32	dram_stack_addr;
	u32	dram_stack_size;

	u32	output_buff_addr;
	u32	output_buff_size_addr;

	u32	data_ptr_addr;
	u32	data_size;

	u32	yield_data_ptr_addr;
	u32	yield_data_size;

};

rspregs rsp;
u32 sp_regs[13] = {3,3,3,3,3,0};

u32 last_dma_sizes[2] = {0};

void sp_run_rsp()
{
	if( last_dma_sizes[0] != 64 || last_dma_sizes[1] != 208 )
	{
		printf("SP: Running non-HLE uCode.\n");
		return;
	}

	rsptask T;
	u32* temp = (u32*)&T;

	for(int i = 0; i < 16; ++i)
		temp[i] = __builtin_bswap32(*(u32*)(DMEM+0xfc0+(i*4)));

	switch(T.type)
	{
	case 2: puts("SP: Running HLE Audio Task"); break;
	case 1: puts("SP: Running HLE Gfx Task"); break;
	default: printf("SP: Unknown HLE Task Type %i\n", T.type); break;
	}

	



	rsp_running = false;
	sp_regs[4] = 3 | BIT(9); //signal 2? halt and broke
	mi_regs[2] |= 1;

	return;
}

void sp_reg_write32(u32 addr, u32 val)
{
	if( addr == 0x04080000 )
	{
		printf("RSP PC set to %x\n", val);
		rsp.PC = val&0xffc;
		return;
	}

	addr &= 0x3F;
	addr >>= 2;
	printf("SP Write: %x to %x\n", val, addr);

	if( addr > 12 ) return;

	if( addr == 2 )
	{
		printf("SP: DMA %i at %x to %x\n", val+1, sp_regs[1], sp_regs[0]);
		u32 start_addr = sp_regs[1]&0x7fffff;
		u8* mem = (sp_regs[0]&0x1000) ? IMEM : DMEM;
		mem += sp_regs[0]&0xfff;

		u32 L = val&0xfff;
		u32 C = (val>>12)&0xff;
		u32 S = (val>>20);

		C++; L++;

		last_dma_sizes[(sp_regs[0]&0x1000)?1:0] = L;

		for(int i = 0; i < C; ++i)
		{
			memcpy(mem, DRAM+start_addr, L);
			mem += L+S;
		}
		return;		
	}

	if( addr == 4 )
	{
		if( val & BIT(0) ) sp_regs[4] &= ~BIT(0);
		else if( val & BIT(1) ) sp_regs[4] |= BIT(0);
		if( val & BIT(2) ) sp_regs[4] &= ~BIT(1);
		if( val & BIT(3) )
		{
			//clear interrupt
			mi_regs[2] &= ~1;
		} else if( val & BIT(4) ) {
			//SET interrupt (huh?)
		}
		if( val & BIT(5) ) sp_regs[4] &= ~BIT(5);
		else if( val & BIT(6) ) sp_regs[4] |= BIT(5);
		if( val & BIT(7) ) sp_regs[4] &= ~BIT(6);
		else if( val & BIT(8) ) sp_regs[4] |= BIT(6);
		if( val & BIT(9) ) sp_regs[4] &= ~BIT(7);
		else if( val & BIT(10) ) sp_regs[4] |= BIT(7);
		if( val & BIT(11) ) sp_regs[4] &= ~BIT(8);
		else if( val & BIT(12) ) sp_regs[4] |= BIT(8);
		if( val & BIT(13) ) sp_regs[4] &= ~BIT(9);
		else if( val & BIT(14) ) sp_regs[4] |= BIT(9);
		if( val & BIT(15) ) sp_regs[4] &= ~BIT(10);
		else if( val & BIT(16) ) sp_regs[4] |= BIT(10);
		if( val & BIT(17) ) sp_regs[4] &= ~BIT(11);
		else if( val & BIT(18) ) sp_regs[4] |= BIT(11);
		if( val & BIT(19) ) sp_regs[4] &= ~BIT(12);
		else if( val & BIT(20) ) sp_regs[4] |= BIT(12);
		if( val & BIT(21) ) sp_regs[4] &= ~BIT(13);
		else if( val & BIT(22) ) sp_regs[4] |= BIT(13);
		if( val & BIT(23) ) sp_regs[4] &= ~BIT(14);
		else if( val & BIT(24) ) sp_regs[4] |= BIT(14);

		printf("SP: After write to SP_STATUS_REG, it=%x\n", sp_regs[4]);

		if( (sp_regs[4]&3)==0 && !rsp_running )
		{
			sp_run_rsp();
		}

		return;
	}

	sp_regs[addr] = val;

	return;
}

u32 sp_reg_read32(u32 addr)
{
	if( addr == 0x04080000 )
	{
		printf("RSP PC read @%x\n", rsp.PC);
		return rsp.PC;
	}

	addr &= 0x3F;
	addr >>= 2;


	return sp_regs[addr];
}

void sp_update(int cpu_cycles)
{

	return;
}



