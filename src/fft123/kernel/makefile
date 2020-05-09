#
#  kernel library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = ..\api;$(TOPDIR)\include
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: kernel$(LBO) $(MDUMMY)

OBJECTS = align$(CO) alloc$(CO) assert$(CO) awake$(CO) buffered$(CO) ct$(CO) \
	hash$(CO) iabs$(CO) md5$(CO) md5_1$(CO) minmax$(CO) ops$(CO) \
	pickdim$(CO) plan$(CO) planner$(CO) primes$(CO) print$(CO) problem$(CO) \
	rader$(CO) scan$(CO) solver$(CO) solvtab$(CO) square$(CO) stride$(CO) \
	tensor$(CO) tensor1$(CO) tensor2$(CO) tensor4$(CO) tensor5$(CO) tensor7$(CO) \
	tensor8$(CO) tensor9$(CO) transpose$(CO) trig$(CO) trig1$(CO) \
	twiddle$(CO) timer$(CO)

kernel$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del kernel$(LBO) $(OBJECTS) $(OBJECTS) 2> nul
