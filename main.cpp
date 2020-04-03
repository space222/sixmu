#include <stdio.h>
#include <cstring>
#include <SDL.h>
#include <cmath>
#include <chrono>
#include <thread>
#include "types.h"
#include "cxxopts.hpp"

SDL_Surface* MainWindowSurf;
bool MainRunning = true;
bool UsingInterpreter = false;

bool load_rom(const std::string&);
int cpu_run();
int interp_cpu_run();
void interp_rsp_run();
bool system_update(int);
void ai_update(int);
void vi_init();
void ai_init();

extern u8 PIF[0x800];
extern regs cpu;
extern rspregs rsp;
extern u8* ROM;
extern u32 rom_size;
extern u8 DRAM[8*1024*1024];
extern u32 sp_regs[13];

int main(int argc, char** args)
{
	bool emupif = true;

	cxxopts::Options options("Sixmu", "Sixmu, the N64 Emulator");
	options.add_options()
		("b,bypass", "Bypass PIF/Bootcode (programs using libultra will fail)")
		("d,debug", "Activate debugger (unimplemented, will require interpreter)")
		("i,interpret", "Use the interpreter instead of dynarec")
		("l,lle", "Use the RSP interpreter even if a libultra task is detected")
		("f,file", "ROM to emulate", cxxopts::value<std::string>())
			;
	options.parse_positional({"file"});

	auto result = options.parse(argc, args);
	if( result.count("b") )
	{
		emupif = false;
	}
	if( result.count("i") )
	{
		UsingInterpreter = true;
	}
	if( !result.count("f") )
	{
		std::cout << options.help() << std::endl;
		return 0;
	}

	if( !load_rom(result["f"].as<std::string>()) )
	{
		return 1;
	}
/*
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
*/
	//memset(&cpu, 0, sizeof(cpu));
	
	//for(int i = 0; i < 0x800000; ++i) DRAM[i] = 0xff;

	u32 sum = 0;
	for(int i = 0x40; i < 0x1000; i++) sum += ROM[i];
	printf("Bootcode simple sum = %x\n", sum);

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
		cpu.PC = __builtin_bswap32(*(u32*)(ROM+8));

		if( sum == 0x371cc ) cpu.PC -= 0x200000;

		u32 entry = cpu.PC;

		printf("info: using entry point 0x%x\n", entry);

		entry &= 0x1FFFFFFF;

		memcpy(DRAM+entry, ROM+0x1000, (0x100000 > rom_size-0x1000) ? rom_size-0x1000 : 0x100000);
	}

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);

	SDL_Window* MainWindow = SDL_CreateWindow("Sixmu", 0, 0, 1300, 960, SDL_WINDOW_OPENGL);
	MainWindowSurf = SDL_GetWindowSurface(MainWindow);

	cpu.R[0] = 0;
	cpu.C[CP0_Status] |= CP0_STATUS_FR;
	rsp.VCO = rsp.VCE = rsp.VCC = 0;
	vi_init();
	ai_init();

	for(int i = 0x7C0; i < 0x800; ++i) PIF[i] = 0;

	// will need to crc32 the bootcode to determine the correct seed
	// at the moment if the value is non-bswapped 0x3f, the recompiler
	// goes nuts the moment the bootcode jumps to RSP mem.
	// for now the recompiler is ignoring the specific infinite loop instruction
	// that prevents booting with incorrect checksum
	u32 bootcheck = 0;
	switch( sum )
	{
	case 0x33a27: bootcheck = 0x3f3f; break; 	 //2 CIC_6101/TWINE
	case 0x3421e: bootcheck = 0x3f3f; break;	 //2 CIC_6101/Starfox
	case 0x34044: bootcheck = 0x3f3f; break;	 //6 CIC_6102/Mario
	case 0x357d0: bootcheck = 0x783f; break;        //2 CIC_6103/Banjo
	case 0x47a81: bootcheck = 0x913f; break;        //2 CIC_6105/Zelda
	case 0x371cc: bootcheck = 0x853f; break; 	 //2 CIC_6106/F-Zero
	case 0x343c9: // CIC_6106 ???
	default: bootcheck = 0x3f3f; break;  //2
	}

	*(u32*)(PIF + 0x7E4) = __builtin_bswap32(bootcheck);
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

		if( !UsingInterpreter )
		{
			int cc;
			do
			{
				cc = cpu_run()/4;
				ai_update(cc);
			} while( !system_update(cc) );
		} else {
			int cc = 0;
			do {
				interp_cpu_run();
				if( (sp_regs[4]&3) == 0 ) interp_rsp_run();
				cc++;
				if( cc >= 547 )
				{
					ai_update(cc);
					cc = 0;
				}
			} while( !system_update(1) );
		}
		SDL_UpdateWindowSurface(MainWindow);	
	}

	SDL_Quit();

	return 0;
}



