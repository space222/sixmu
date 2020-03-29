#include <SDL.h>
#include <cstring>
#include <cstdlib>
#include "types.h"

SDL_Surface* vi_screen;
extern SDL_Surface* MainWindowSurf;

u32 lookup[0x10000];
u16 bssurf[640*480];

extern u32 mi_regs[4];
u32 vi_regs[14];

u32 last_format = 0;
u32 last_width = 0;
u32 last_height = 0;
u32 line_cycles = 0;

extern u8 DRAM[8*1024*1024];

void vi_reg_write32(u32 addr, u32 val)
{
	addr &= 0x3F;
	//printf("VI: Write %x to %x\n", val, addr);
	addr >>= 2;

	if( addr > 13 ) return;

	if( addr == 4 )
	{
		// write to VI_V_CURRENT_LINE_REG clears scanline interrupt
		mi_regs[2] &= ~BIT(3);
		//printf("VI: Ack Interrupt\n");
		return;
	}

	//printf("VI write 0x%x to reg %x\n", val, addr<<2);

	vi_regs[addr] = val;

	return;
}

u32 vi_reg_read32(u32 addr)
{
	addr &= 0x3F;
	addr >>= 2;

	if( addr > 13 ) return 0;

	return vi_regs[addr];
}

bool vi_update(int cpu_cycles)
{
	bool update_screen = false;
	line_cycles += cpu_cycles;

	if( line_cycles >= 548 )  // 93.75Mhz / 60 / 525 scanlines = ~2975, with the approximation of 1 instruction per cycle
	{
		line_cycles -= 548;
		vi_regs[4]++;
		if( vi_regs[4] == vi_regs[3] )
		{
			mi_regs[2] |= BIT(3);
		} 
		if( vi_regs[4] == 525 ) 
		{
			vi_regs[4] = 0;
			update_screen = true;
		}
	}

	if( !update_screen ) return false;

	if( (vi_regs[0]&3) == 0 || vi_regs[2] == 0 ) return true;

	if( last_format != (vi_regs[0]&3) )
	{
		last_format = (vi_regs[0] & 3);
		last_width = vi_regs[2];
		last_height = (last_width == 320) ? 240 : 480;

		SDL_FreeSurface(vi_screen);

		if( last_format == 3 )
		{
			vi_screen = SDL_CreateRGBSurface(0, last_width, last_height, 32, 0xff, 0xff00, 0xff0000, 0xff000000);// 0xff000000, 0xff0000, 0xff00, 0xff);
		} else {
			vi_screen = SDL_CreateRGBSurface(0, last_width, last_height, 16, 0x7c00, 0x03e0, 0x001F, 0x8000);
		}

		printf("VI_V_START: start = %i, end = %i\n", vi_regs[10]>>16, vi_regs[10]&0x3FF);

		SDL_SetSurfaceBlendMode(vi_screen, SDL_BLENDMODE_NONE);

		if( !vi_screen )
		{
			printf("FATAL: Unable to create SDL surface.\n");
			exit(1);
		}
	}

	u32 src = vi_regs[1] & 0x1FFFFFFF;
	int pixsize = (last_format==3) ? 4 : 2;
	u8* srcp = DRAM+src;

	if( pixsize == 2 )
	{
		u16* temp = (u16*) srcp;
		for(int i = 0; i < last_width*last_height; ++i)
		{
			bssurf[i] = __builtin_bswap16(temp[i]);
		}
		srcp =(u8*) (bssurf+0);
	}

	SDL_LockSurface(vi_screen);

	if( vi_screen->pitch == last_width*pixsize )
	{
		memcpy(vi_screen->pixels, srcp, last_width*last_height*pixsize);
	} else {
		for(int i = 0; i < last_height; ++i)
		{
			u8* dest =(u8*) vi_screen->pixels;
			memcpy(dest + i*vi_screen->pitch, srcp+(i*last_width*pixsize), last_width*pixsize);
		}
	}

	SDL_UnlockSurface(vi_screen);

	SDL_Rect rect; rect.x = rect.y = 0; rect.w = 640; rect.h = 480;
	SDL_BlitScaled(vi_screen, nullptr, MainWindowSurf, &rect);
	//SDL_Rect rect; rect.x = rect.y = 0; rect.w = last_width; rect.h = last_height;
	//SDL_BlitSurface(vi_screen, nullptr, MainWindowSurf, &rect);

	return true;
}

void vi_init()
{
	for(int i = 0; i < 14; ++i) 
	{
		vi_regs[i] = 0;
	}

	last_format = 0;
	last_width = 640;
	last_height = 480;

//	vi_screen = SDL_CreateRGBSurface(0, 480, 640, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0xff);
//	SDL_SetSurfaceBlendMode(vi_screen, SDL_BLENDMODE_NONE);
	return;
}

