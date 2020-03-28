#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "types.h"

extern regs cpu;
extern u32 mi_regs[4];
extern int branch_delay;
extern bool UsingInterpreter;

void ai_update(int);
void pi_update(int);
bool vi_update(int);

bool once = true;

bool system_update(int cpu_cycles)
{
	pi_update(cpu_cycles);
	bool do_window_update = vi_update(cpu_cycles);

	if( UsingInterpreter )
	{
		cpu.C[CP0_Count]++;
		if( cpu.C[CP0_Count] == cpu.C[CP0_Compare] )
		{
			cpu.C[CP0_Cause] |= BIT(15);
		}
	} else {
		if( cpu.C[CP0_Count] < cpu.C[CP0_Compare] && cpu.C[CP0_Count] + cpu_cycles >= cpu.C[CP0_Compare] )
		{
			// handle some amount of rollover?	
	//		cpu.C[CP0_Count] = cpu.C[CP0_Compare];
	//		cpu.C[CP0_Cause] |= BIT(15);
		} else if( ((cpu.C[CP0_Count]>>31)&1) && !((cpu.C[CP0_Count]+(cpu_cycles<<4))>>31&1) )  {
	//		cpu.C[CP0_Count] = cpu.C[CP0_Compare] = 0;
	//		cpu.C[CP0_Cause] |= BIT(15);
		} else {
			cpu.C[CP0_Count] += cpu_cycles<<4;
		}
	}

	if( mi_regs[3]&mi_regs[2]&0x3F )
	{
		cpu.C[CP0_Cause] |= BIT(10);
	} else {
		cpu.C[CP0_Cause] &= ~BIT(10);
	}

	if( (cpu.C[CP0_Cause]&cpu.C[CP0_Status]&0xFF00) && ((cpu.C[CP0_Status]&3)==1) ) 
	{
		cpu.C[CP0_EPC] = cpu.PC;
		if( branch_delay ) cpu.C[CP0_EPC] |= BIT(31);
		cpu.C[CP0_Status] |= 2;
		cpu.PC = 0x80000180;
	}

	return do_window_update;	
}


