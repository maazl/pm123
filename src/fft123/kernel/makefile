#
#  kernel library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = ..\api;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

all: kernel$(LBO) $(MDUMMY)

OBJECTS = align$(CO) alloc$(CO) assert$(CO) awake$(CO) buffered$(CO) ct$(CO)
OBJECTS = $(OBJECTS) hash$(CO) iabs$(CO) md5$(CO) md5_1$(CO) minmax$(CO) ops$(CO)
OBJECTS = $(OBJECTS) pickdim$(CO) plan$(CO) planner$(CO) primes$(CO) print$(CO) problem$(CO)
OBJECTS = $(OBJECTS) rader$(CO) scan$(CO) solver$(CO) solvtab$(CO) square$(CO) stride$(CO)
OBJECTS = $(OBJECTS) tensor$(CO) tensor1$(CO) tensor2$(CO) tensor4$(CO) tensor5$(CO) tensor7$(CO)
OBJECTS = $(OBJECTS) tensor8$(CO) tensor9$(CO) transpose$(CO) trig$(CO) trig1$(CO)
OBJECTS = $(OBJECTS) twiddle$(CO) timer$(CO)

kernel$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del kernel$(LBO) $(OBJECTS) $(OBJECTS) 2> nul
