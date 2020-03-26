#include <cstdio>
#include <string>
#include "types.h"


extern u8* ROM;
extern u32 rom_size;


bool load_rom(char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if( ! fp )
	{
		printf("Error: unable to open '%s'\n", filename);
		return false;
	}

	fseek(fp, 0, SEEK_END);
	rom_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	ROM = new u8[rom_size];
	int unu = fread(ROM, 1, rom_size, fp);
	fclose(fp);

	u32 F = *(u32*)ROM;
	if( F == 0x12408037 )
	{
		puts("info: ROM is byte-swapped");
		u16* temp = (u16*) ROM;
		for(int i = 0; i < (rom_size>>1); ++i)
		{
			temp[i] = (temp[i]<<8)|(temp[i]>>8);
		}
	} else if( F == 0x40123780 ) {
		puts("info: ROM byte order is good");
	} else {
		puts("error: not an n64 rom or unimplemented byte order");
		return false;
	}

	//printf("ROM name = '");
	//for(int i = 0; i < 20; ++i)
	//	putchar(ROM[0x20+i]);
	//printf("'\n");

	printf("Entry point = 0x%x\n", __builtin_bswap32(*(u32*)(ROM+8)));
	return true;
}

