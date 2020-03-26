# sixmu
A Nintendo 64 (n64) emulator in C++; the recompiler uses libfmt and libtcc to compile C source on the fly.

# status
A few demos run. Most roms that use libultra (in order of likeliness): handle the VI interrupt but nothing else happens, no
indication (only running previously recomp'd code), crash in the weeds, or run (rotate/zoom demo only). Other demos
that don't use libultra (or maybe use it unconventionally?) are most likely to run.

All of the krom's CPU/CP1 instruction [tests](https://github.com/PeterLemon/N64) pass.

See todo.txt for more in-depth information.

