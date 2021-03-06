PROJDIRS := .
SRCFILES := $(shell find . -name "*.cpp")
OBJFILES := $(addprefix ./build/, $(patsubst %.cpp,%.o,$(notdir $(SRCFILES))))
DEPFILES := $(patsubst %.o,%.d,$(OBJFILES))
CPPFLAGS := `pkg-config --cflags sdl2` -O2 -std=c++17 -I./tcc -I.
LIBS := `pkg-config --libs sdl2` libtcc.a libtcc1.a libfmt.a -ldl

all: a.out

a.out : $(OBJFILES)
	@echo "Linking..."
	@g++ -o a.out $(OBJFILES) $(LIBS)
	@echo "done."

./build/%.o: %.cpp Makefile
	g++ $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJFILES) : | ./build

./build : 
	mkdir ./build

-include $(DEPFILES)


