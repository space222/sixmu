#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "types.h"

extern u32 mi_regs[4];
extern u8 DRAM[8*1024*1024];
extern u8 DMEM[0x1000];

u32 dp_regs[8];

rdp_other_modes other_modes;

struct {
	u32 format;
	u32 pixsize;
	u32 width;
	u32 addr;
} color_buffer;

struct {
	u32 xh, yh, xl, yl;
	bool interlace;
	bool keep_odd_line;
} rdp_scissor;

u32 zbuf_addr = 0;
u32 fill_color = 0;
u32 env_color = 0;
u32 blend_color = 0;
u32 fog_color = 0;
u32 primitive_color = 0;
u32 primitive_z = 0;
u32 primitive_z_offset = 0;

void dp_fill_rect(u32 xh, u32 yh, u32 xl, u32 yl);
void dp_set_other_modes(u64 first);

void dp_do_commands()
{
	u32 start = dp_regs[2];
	u32 end = dp_regs[1];

	printf("DP EXECUTE FROM %x TO %x\n", start, end);

	while( start < end )
	{
		u64 first;
		if( dp_regs[3] & 1 )
			first = __builtin_bswap64(*(u64*)(DMEM+(start&0x7FFFFF)));
		else
			first = __builtin_bswap64(*(u64*)(DRAM+(start&0x7FFFFF)));
		u8 cmd = (first>>56)&0x3F;

		switch( cmd )
		{
		case 0x00: // NOP
			puts("DP CMD: NOP");
			break;
		case 0x3F: // Set Color Image
			color_buffer.format = (first>>53)&7;
			color_buffer.width = ((first>>32)&0x3FF) + 1;
			color_buffer.pixsize = (first>>51)&3;
			color_buffer.addr = (first & 0x7FFFFF);
			printf("DP CMD: Set Color Image = 0x%x\n", color_buffer.addr);
			break;
		case 0x3E: // Set Z Image
			zbuf_addr = (first & 0x7FFFFF);
			printf("DP CMD: Set Z Image = 0x%x\n", zbuf_addr);
			break;
		case 0x2D: // Set Scissor
			rdp_scissor.xh = (first>>44)&0xFFF;
			rdp_scissor.yh = (first>>32)&0xFFF;
			rdp_scissor.xl = (first>>12)&0xFFF;
			rdp_scissor.yl = (first & 0xFFF);
			rdp_scissor.interlace = (first>>25)&1;
			rdp_scissor.keep_odd_line = (first>>24)&1;
			printf("DP CMD: Set Scissor (%i,%i,%i,%i)\n", rdp_scissor.xl, rdp_scissor.yl, rdp_scissor.xh, rdp_scissor.yh);
			break;
		case 0x37: // Set Fill Color
			fill_color =(u32) first;
			printf("DP CMD: Fill Color = %x\n", fill_color);
			break;
		case 0x36: // Fill Rectangle
			dp_fill_rect((first>>12)&0xFFF, first&0xFFF, (first>>44)&0xFFF, (first>>32)&0xFFF);
			break;
		case 0x27: // Sync Pipe
			// sync commands are pointless to an implementation where things happen instantly
			printf("DP CMD: Sync Pipe\n");
			break;
		case 0x28: // Sync Tile
			printf("DP CMD: Sync Tile\n");
			break;
		case 0x29: // Sync Full
			printf("DP CMD: Sync Full\n");
			break;
		case 0x38: // Set Fog Color
			fog_color =(u32) first;
			break;
		case 0x3B: // Set Env Color
			env_color =(u32) first;
			break;
		case 0x39: // Set Blend Color
			blend_color =(u32) first;
			break;
		case 0x2E: // Set Primitive Depth
			primitive_z = (first>>16)&0xFFFF;
			primitive_z_offset = first&0xFFFF;
			break;
		case 0x2F: // Set Other Modes
			dp_set_other_modes(first);
			puts("DP CMD: Set Other Modes");
			break;
		default:
			printf("DP CMD 0x%x\n", cmd);
			break;		
		}

		start += 8;
	}

	return;
}

void dp_fill_rect(u32 xh, u32 yh, u32 xl, u32 yl)
{
	u32 fill_val = 0;
	if( color_buffer.pixsize == 3 )
	{
		fill_val = __builtin_bswap32(fill_color);
	} else {
		fill_val = __builtin_bswap16(fill_color&0xffff);
		fill_val |= __builtin_bswap16(fill_color>>16)<<16;
	}

	xh >>= 2; 
	yh >>= 2; 
	xl >>= 2; 
	yl >>= 2;

	printf("DP: fill rect %i, %i, %i, %i\n", xh, yh, xl, yl);

	if( color_buffer.pixsize == 3 )
	{
		for(int X = xh; X <= xl; ++X)
		{
			for(int Y = yh; Y <= yl; ++Y)
			{
				u32 offset = Y*color_buffer.width + X;
				*(u32*)(DRAM+color_buffer.addr+offset*4) = fill_val;
			}
		}
	} else if( color_buffer.pixsize == 2 ) {
		for(int X = xh; X <= xl; ++X)
		{
			for(int Y = yh; Y <= yl; ++Y)
			{
				u32 offset = Y*color_buffer.width + X;
				*(u16*)(DRAM+color_buffer.addr+offset*2) = (offset&1) ? (u16)fill_val : (u16)(fill_val>>16);
			}
		}
	}
	return;
}

void dp_reg_write32(u32 addr, u32 val)
{
	addr &= 0x3F;
	addr >>= 2;

	//printf("DP REG WRITE, %x to %x\n", val, addr);

	if( addr > 7 ) return;
	if( addr == 2 ) return;

	if( addr == 0 )
	{
		dp_regs[0] = val&0x7FFFF;
	} else if( addr == 1 ) {
		dp_regs[1] = val&0x7FFFFF;
	} else if( addr == 3 ) {
		if( val & BIT(0) ) { dp_regs[3] &= ~BIT(0); }
		else if( val & BIT(1) ) { dp_regs[3] |= BIT(0); }
	} else {
		dp_regs[addr] = val;
	}

	if( dp_regs[1] > dp_regs[0] )
	{
		if( dp_regs[2] < dp_regs[0] || dp_regs[2] > dp_regs[1] )
		{
			dp_regs[2] = dp_regs[0];
		}
		if( dp_regs[2] < dp_regs[1] )
		{
			dp_do_commands();
			dp_regs[3] |= 0x80; // dp commands are completed before returning (inaccurately). RDP never busy.
			dp_regs[2] = dp_regs[1];
		}
	}
	return;
}

u32 dp_reg_read32(u32 addr)
{
	addr &= 0x3F;
	addr >>= 2;

	if( addr > 7 ) return 0;

	return dp_regs[addr];
}

void dp_set_other_modes(u64 first)
{
	other_modes.atomic_primitive = (first>>55)&1;
	other_modes.cycle_type = (first>>52)&3;
	other_modes.perspective_correct = (first>>51)&1;
	other_modes.detail_texture = (first>>50)&1;
	other_modes.sharpen_texture = (first>>49)&1;
	other_modes.enable_texture_lod = (first>>48)&1;
	other_modes.tlut_enable = (first>>47)&1;
	other_modes.tlut_format = (first>>46)&1;
	other_modes.sample_type = (first>>45)&1;
	other_modes.mid_texel_enable = (first>>44)&1;
	other_modes.bilinear_interp_0 = (first>>43)&1;
	other_modes.bilinear_interp_1 = (first>>42)&1;
	other_modes.color_convert = (first>>41)&1;
	other_modes.rgb_dither = (first>>38)&3;
	other_modes.alpha_dither=(first>>36)&3;
	other_modes.blend_m1a0 = (first>>30)&3;
	other_modes.blend_m1a1 = (first>>28)&3;
	other_modes.blend_m1b0 = (first>>26)&3;
	other_modes.blend_m1b1 = (first>>24)&3;
	other_modes.blend_m2a0 = (first>>22)&3;
	other_modes.blend_m2a1 = (first>>20)&3;
	other_modes.blend_m2b0 = (first>>18)&3;
	other_modes.blend_m2b1 = (first>>16)&3;
	other_modes.force_blend = (first>>14)&1;
	other_modes.enable_coverage = (first>>13)&1;
	other_modes.alpha_multiply = (first>>12)&1;
	other_modes.z_mode = (first>>10)&3;
	other_modes.coverage_mode = (first>>8)&3;
	other_modes.color_on_coverage = (first>>7)&1;
	other_modes.image_read_enable = (first>>6)&1;
	other_modes.update_z = (first>>5)&1;
	other_modes.z_compare = (first>>4)&1;
	other_modes.anti_alias = (first>>3)&1;
	other_modes.z_source = (first>>2)&1;
	other_modes.alpha_comp_type = (first>>1)&1;
	other_modes.alpha_comp_enable = (first&1);
	return;
}


