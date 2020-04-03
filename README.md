# sixmu
A Nintendo 64 (n64) emulator in C++; the recompiler uses libfmt and libtcc to compile C source on the fly.

# status

Instead of spending extra time debugging the recompiler, I decided to actually write an interpreter and get
back to the recomp later. I've added the '-i' option to turn on the interpreter, otherwise the recompiler
gets used. The '-b' is now bypass the PIF (otherwise requires pifdata.bin to be in the same folder). Bypass
is not capable of simulating the results of the bootcode, so it is only useful for krom's test ROMs.

Some commercial games get to the point of running an audio task on the RSP. Many demos get to the point
of running a graphics task on the RSP. This is the point where I attempt to write my own HLE code.
A few games poke a logo into the framebuffer before not doing anything else, which was fun to see.

Most roms that use libultra (in order of likeliness): pause waiting for HLEable task completion, no
indication (only running previously recomp'd code, or not triggering any debug prints in the interpreter), 
crash in the weeds (only like 2 ROMs in the interpeter), or run (showing some kind of gfx or maybe 
waiting for input). Other demos that don't use libultra (or maybe use it unconventionally?) are most likely to run.

All of krom's CPU/CP1 instruction [tests](https://github.com/PeterLemon/N64) pass for both the recompiler
and interpreter. All of his RSP SU tests pass. I've started the vector unit.

See todo.txt for more in-depth information.

