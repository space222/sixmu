#include <cstdio>
#include <cstring>
#include "types.h"

extern bool UsingInterpreter;

u8 DMEM[0x1000];
u8 IMEM[0x1000];
u8 DRAM[8*1024*1024];
u8 PIF[0x800] = {0};

u8* ROM;
u32 rom_size;

void invalidate_code(u32);

u8 read_rom8(u32 addr) 
{ 
	addr &= 0x0FFFFFFF;

	if( addr >= rom_size ) return 0xff;

	return ROM[addr];
}

u16 read_rom16(u32 addr)
{
	addr &= 0x0FFFFFFE;

	if( addr >= rom_size ) return 0xffff;

	return __builtin_bswap16(*(u16*)(ROM+addr));
}

u32 read_rom32(u32 addr)
{
	addr &= 0x0FFFFFFC;

	if( addr >= rom_size ) return 0xffffffff;

	return __builtin_bswap32(*(u32*)(ROM+addr));
}

u64 read_rom64(u32 addr)
{
	//printf("ROM: read64 @%x\n", addr);
	addr &= 0x0FFFFFF8;

	if( addr >= rom_size ) return 0xabcdef5;

	return __builtin_bswap64(*(u64*)(ROM+addr));
}

void vi_reg_write32(u32, u32);
u32 vi_reg_read32(u32);
void mi_reg_write32(u32, u32);
u32 mi_reg_read32(u32);
void dp_reg_write32(u32, u32);
u32 dp_reg_read32(u32);
void ai_reg_write32(u32, u32);
u32 ai_reg_read32(u32);
void pi_reg_write32(u32, u32);
u32 pi_reg_read32(u32);
void sp_reg_write32(u32, u32);
u32 sp_reg_read32(u32);
void si_reg_write32(u32, u32);
u32 si_reg_read32(u32);

u64 read64(u32 addr)
{
	if( addr < 0x80000000 ) return 0xff;

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 ) return __builtin_bswap64(*(u64*)(DRAM+(addr&0x7FFFF8)));
		
		if( addr >= 0x04000000 && addr < 0x04001000 ) return __builtin_bswap64(*(u64*)(DMEM+(addr&0xff8)));
		if( addr >= 0x04001000 && addr < 0x04002000 ) return __builtin_bswap64(*(u64*)(IMEM+(addr&0xff8)));

		if( addr >= 0x10000000 && addr < 0x1FC00000 ) return read_rom64(addr);

		if( addr >= 0x1FC00000 ) 
		{
			//todo: there's something that when read alters a byte elsewhere on boot
			return  __builtin_bswap64(*(u64*)(PIF+(addr&0x7f8)));
		}
	} else {
		//todo: TLB
	}

	return 0xccbbaabb;
}

u32 read32(u32 addr)
{
	//printf("READ32 @%x\n", addr);

	if( addr < 0x80000000 ) return 0xff;

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 ) return  __builtin_bswap32(*(u32*)(DRAM+(addr&0x7FFFFC)));
		
		if( addr >= 0x04000000 && addr < 0x04001000 ) return __builtin_bswap32(*(u32*)(DMEM+(addr&0xffc)));
		if( addr >= 0x04001000 && addr < 0x04002000 ) return __builtin_bswap32(*(u32*)(IMEM+(addr&0xffc)));
		if( addr >= 0x04040000 && addr < 0x04080008 ) return sp_reg_read32(addr);
		if( addr >= 0x04100000 && addr < 0x04100020 ) return dp_reg_read32(addr);
		if( addr >= 0x04300000 && addr < 0x04300010 ) return mi_reg_read32(addr);
		if( addr >= 0x04400000 && addr < 0x04400038 ) return vi_reg_read32(addr);
		if( addr >= 0x04500000 && addr < 0x04500018 ) return ai_reg_read32(addr);
		if( addr >= 0x04600000 && addr < 0x04600034 ) return pi_reg_read32(addr);
		if( addr >= 0x04800000 && addr < 0x0480001C ) return si_reg_read32(addr);
		
		//if( addr >= 0x06000000 && addr < 0x10000000 ) return read_rom32(addr);

		if( addr >= 0x10000000 && addr < 0x1FC00000 ) return read_rom32(addr);

		if( addr >= 0x1FC00000 )
		{
			//if( (addr&0x7fc) >= 0x7c0 )
			//	printf("PIF Read @%x = %x\n", addr, __builtin_bswap32(*(u32*)(PIF+(addr&0x7fc))));
			if( (addr&0x7fc) == 0x7e4 ) PIF[0x7ff] = 0x80;
			return  __builtin_bswap32(*(u32*)(PIF+(addr&0x7fc)));
		}
	} else {
		//todo: TLB
	}

	return 0xbadbeef;
}

u16 read16(u32 addr)
{
	if( addr < 0x80000000 ) return 0xff;

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 ) return  __builtin_bswap16(*(u16*)(DRAM+(addr&0x7FFFFe)));
		
		if( addr >= 0x04000000 && addr < 0x04001000 ) return  __builtin_bswap16(*(u16*)(DMEM+(addr&0xffe)));
		if( addr >= 0x04001000 && addr < 0x04002000 ) return  __builtin_bswap16(*(u16*)(IMEM+(addr&0xffe)));

		if( addr >= 0x10000000 && addr < 0x1FC00000 ) return read_rom16(addr);

		if( addr >= 0x1FC00000 ) 
		{
			return  __builtin_bswap16(*(u16*)(PIF+(addr&0x7fe)));
		}
	} else {
		//todo: TLB
	}

	return 0xbaca;
}


u8 read8(u32 addr)
{
	if( addr < 0x80000000 ) return 0xff;  //?

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 ) return DRAM[addr&0x7FFFFF];
		
		if( addr >= 0x04000000 && addr < 0x04001000 ) return DMEM[addr&0xfff];
		if( addr >= 0x04001000 && addr < 0x04002000 ) return IMEM[addr&0xfff];

		if( addr >= 0x10000000 && addr < 0x1FC00000 ) return read_rom8(addr);

		if( addr >= 0x1FC00000 ) 
		{
			return PIF[addr&0x7ff];
		}
	} else {
		//todo: TLB
	}

	return 0;
}


void write8(u32 addr, u8 val)
{
	if( addr < 0x80000000 ) return;  //?

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 ) 
		{
			DRAM[addr] = val;
			if( !UsingInterpreter ) invalidate_code(addr);
		}
		
		else if( addr >= 0x04000000 && addr < 0x04001000 ) DMEM[addr&0xfff] = val;
		else if( addr >= 0x04001000 && addr < 0x04002000 ) IMEM[addr&0xfff] = val;

		else if( addr >= 0x10000000 && addr < 0x1FC00000 ) return;

		else if( addr >= 0x1FC00000 ) 
		{
			PIF[addr&0x7ff] = val;
		}
	} else {
		//todo: TLB
	}

	return;
}

void write16(u32 addr, u16 val)
{
	if( addr < 0x80000000 ) return;  //?

	val = __builtin_bswap16(val);

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 )
		{
			memcpy((DRAM+(addr&0x7FFFFe)), &val, 2);
			if( !UsingInterpreter ) invalidate_code(addr);
		}
		
		else if( addr >= 0x04000000 && addr < 0x04001000 ) *(u16*)(DMEM+(addr&0xffe)) = val;
		else if( addr >= 0x04001000 && addr < 0x04002000 ) *(u16*)(IMEM+(addr&0xffe)) = val;

		else if( addr >= 0x10000000 && addr < 0x1FC00000 ) return;

		else if( addr >= 0x1FC00000 ) 
		{
			*(u16*)(PIF+(addr&0x7fe)) = val;
		}
	} else {
		//todo: TLB
	}

	return;
}

void write32(u32 addr, u32 val)
{
	//printf("WRITE32: @0x%x = 0x%x\n", addr, val);
	if( addr < 0x80000000 ) return;  //?

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 )
		{
			val = __builtin_bswap32(val);
			memcpy((DRAM+(addr&0x7FFFFc)), &val, 4);
			if( !UsingInterpreter ) invalidate_code(addr);
		}

		else if( addr >= 0x04000000 && addr < 0x04001000 ) *(u32*)(DMEM+(addr&0xffc)) = __builtin_bswap32(val);
		else if( addr >= 0x04001000 && addr < 0x04002000 ) *(u32*)(IMEM+(addr&0xffc)) = __builtin_bswap32(val);
		else if( addr >= 0x04040000 && addr < 0x04080008 ) sp_reg_write32(addr, val);
		else if( addr >= 0x04100000 && addr < 0x04100020 ) dp_reg_write32(addr, val);
		else if( addr >= 0x04300000 && addr < 0x04300010 ) mi_reg_write32(addr, val);
		else if( addr >= 0x04400000 && addr < 0x04400038 ) vi_reg_write32(addr, val);
		else if( addr >= 0x04500000 && addr < 0x04500018 ) ai_reg_write32(addr, val);
		else if( addr >= 0x04600000 && addr < 0x04600034 ) pi_reg_write32(addr, val);
		else if( addr >= 0x04800000 && addr < 0x0480001C ) si_reg_write32(addr, val);

		else if( addr >= 0x10000000 && addr < 0x1FC00000 ) return;

		else if( addr >= 0x1FC00000 ) 
		{
			//printf("PIF Write! %x to %x\n", val, addr);
			*(u32*)(PIF+(addr&0x7fc)) = __builtin_bswap32(val);
		}
	} else {
		//todo: TLB
	}

	return;
}

void write64(u32 addr, u64 val)
{
	if( addr < 0x80000000 ) return;  //?

	//printf("WRITE64: 0x%x <- 0x%x\n", addr, val);

	val = __builtin_bswap64(val);

	if( addr < 0xc0000000 )
	{
		//non-TLB mapped, not implementing cache
		addr &= 0x1FFFFFFF;
		if( addr < 0x800000 )
		{
			memcpy((DRAM+(addr&0x7FFFF8)), &val, 8);
			if( !UsingInterpreter ) invalidate_code(addr);
		}
		
		else if( addr >= 0x04000000 && addr < 0x04001000 ) *(u64*)(DMEM+(addr&0xff8)) = val;
		else if( addr >= 0x04001000 && addr < 0x04002000 ) *(u64*)(IMEM+(addr&0xff8)) = val;

		else if( addr >= 0x10000000 && addr < 0x1FC00000 ) return;

		else if( addr >= 0x1FC00000 ) 
		{
			*(u64*)(PIF+(addr&0x7f8)) = val;
		}
	} else {
		//todo: TLB
	}

	return;
}

u32 check_vaddr(u32 vaddr, u32 from)
{
	return from;
}












