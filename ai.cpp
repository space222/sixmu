#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <SDL.h>
#include "types.h"

extern u8 DRAM[8*1024*1024];

void ai_play_buffer(void* src, int byte_len);

u32 ai_regs[5] = {0};
extern u32 mi_regs[4];
SDL_AudioDeviceID ai_dev;
float abuf[0x80000];
bool playing = false;

//todo: proper AI reg double buffer, use the SDL audio callback, etc

void ai_reg_write32(u32 addr, u32 val)
{
	addr &= 0x1F;
	//printf("AI Reg Write: %x to %x\n", val, addr);
	addr >>= 2;

	if( addr > 4 ) return;

	ai_regs[addr] = val;

	if( (addr == 1 || addr == 2) && (ai_regs[1] && (ai_regs[2]&1)) )
	{
		if( SDL_GetQueuedAudioSize(ai_dev) ) return;

		//u16* start = (u16*) (DRAM+(ai_regs[0]&0x7FFFF8));
		ai_play_buffer(DRAM+(ai_regs[0]&0x7FFFF8), ai_regs[1]&0x3FFF8);
		ai_regs[0] = ai_regs[1] = 0;		
	}
	
	return;
}

u32 ai_reg_read32(u32 addr)
{
	addr &= 0x1F;
	addr >>= 2;

	if( addr > 4 || addr == 0 ) return 0;

	if( addr == 1 ) 
	{
		return SDL_GetQueuedAudioSize(ai_dev);
	}

	if( addr == 3 )
	{
		if( SDL_GetQueuedAudioSize(ai_dev) ) return 0x40000000;
		else return 0;
	}

	return ai_regs[addr];
}

void ai_update(int cpu_cycles)
{
	u32 remaining = SDL_GetQueuedAudioSize(ai_dev);

	if( remaining == 0 )
	{
		if( playing )
		{
			playing = false;
			//todo: AI interrupt
			mi_regs[2] |= 4;
		}

		if( ai_regs[1] && (ai_regs[2]&1) )
		{
			ai_play_buffer(DRAM+(ai_regs[0]&0x7FFFF8), ai_regs[1]&0x3FFF8);
		}
	}
	return;
}

void ai_play_buffer(void* src, int byte_len)
{
	if( 0 < SDL_QueueAudio(ai_dev, src, byte_len) )
	{
		printf("AI Error: %s\n", SDL_GetError());
		exit(1);
	}
	
	playing = true;
	return;
}

void ai_init()
{
	// mostly (approaching all) direct from the SDL documentation
	SDL_AudioSpec want, have;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = 22100;
	want.format = AUDIO_S16MSB; //AUDIO_F32;
	want.channels = 2;
	want.samples = 4096;
	want.callback = nullptr;

	ai_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if( ai_dev == 0 )
	{
		printf("Failed to open audio: %s\n", SDL_GetError());
		exit(1);		
	}

	printf("AI: Successful Initialization.\n");
	SDL_PauseAudioDevice(ai_dev, 0); /* start audio playing. */
	return;
}



