#include <stdio.h>
#include <cstring>
#include <SDL.h>
#include <cmath>
#include <chrono>
#include <thread>
#include "types.h"
#include "libtcc.h"

SDL_Surface* MainWindowSurf;
bool MainRunning = true;

bool load_rom(char*);
int cpu_run();
bool system_update(int);
void ai_update(int);
void vi_init();
void ai_init();

extern u8 PIF[0x800];
extern regs cpu;
extern u8* ROM;
extern u32 rom_size;
extern u8 DRAM[8*1024*1024];

int main(int argc, char** args)
{
	if( argc < 2 )
	{
		puts("Usage: sixmu file.n64");
		return 1;
	}

	bool emupif = true;
	int A = 1;
	if( argc > 2 )
	{
		if( "-d" == std::string(args[1]) )
		{
			A++;
			emupif = false;
		}
	}

	if( !load_rom(args[A]) )
	{
		return 1;
	}

	//memset(&cpu, 0, sizeof(cpu));
	
	//for(int i = 0; i < 0x800000; ++i) DRAM[i] = 0xff;

	if( emupif )
	{
		FILE* fp = fopen("pifdata.bin", "rb");
		if( ! fp )
		{
			printf("error: unable to find 'pifdata.bin'.\n");
			printf("Use -d to attempt to run without it.\n");
			return 1;
		}

		int unu = fread(PIF, 1, 0x7C0, fp);
		fclose(fp);
		cpu.PC = 0xBFC00000;
	} else {
		u32 entry = cpu.PC = __builtin_bswap32(*(u32*)(ROM+8));
		printf("info: using entry point 0x%x\n", entry);

		entry &= 0x1FFFFFFF;

		memcpy(DRAM+entry, ROM+0x1000, (0x100000 > rom_size-0x1000) ? rom_size-0x1000 : 0x100000);
	}

	u32 sum = 0;
	for(int i = 0x40; i < 0x1000; ++i) sum += ROM[i];
	printf("Bootcode simple sum = %x\n", sum);

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);

	SDL_Window* MainWindow = SDL_CreateWindow("Sixmu", 0, 0, 1200,720, 0); //SDL_WINDOW_OPENGL
	MainWindowSurf = SDL_GetWindowSurface(MainWindow);

	cpu.R[0] = 0;
	cpu.C[CP0_Status] |= CP0_STATUS_FR;
	vi_init();
	ai_init();

	for(int i = 0x7C0; i < 0x800; ++i) PIF[i] = 0;

	// will need to crc32 the bootcode to determine the correct seed
	// at the moment if the value is non-bswapped 0x3f, the recompiler
	// goes nuts the moment the bootcode jumps to RSP mem.
	// for now the recompiler is ignoring the specific infinite loop instruction
	// that prevents booting with incorrect checksum
	*(u32*)(PIF + 0x7E4) = 0;
	*(u32*)(PIF + 0x7fc) = 0;

	while( MainRunning )
	{
		auto stamp1 = std::chrono::system_clock::now();

		SDL_Event event;
		while( SDL_PollEvent(&event) ) 
		{
			switch( event.type )
			{
			case SDL_QUIT:
				MainRunning = false;
				break;
			}
		}

		int cc;
		do
		{
			cc = cpu_run()/4;
			ai_update(cc);
		} while( !system_update(cc) );

		SDL_UpdateWindowSurface(MainWindow);	
	}

	SDL_Quit();

	return 0;
}



