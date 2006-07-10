#
#  kernel.lib makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\api

!include $(TOPDIR)\..\config\makerules

all: kernel.lib $(MDUMMY)

OBJECTS = align.obj alloc.obj assert.obj awake.obj buffered.obj ct.obj
OBJECTS = $(OBJECTS) hash.obj iabs.obj md5.obj md5_1.obj minmax.obj ops.obj
OBJECTS = $(OBJECTS) pickdim.obj plan.obj planner.obj primes.obj print.obj problem.obj
OBJECTS = $(OBJECTS) rader.obj scan.obj solver.obj solvtab.obj square.obj stride.obj
OBJECTS = $(OBJECTS) tensor.obj tensor1.obj tensor2.obj tensor4.obj tensor5.obj tensor7.obj
OBJECTS = $(OBJECTS) tensor8.obj tensor9.obj transpose.obj trig.obj trig1.obj
OBJECTS = $(OBJECTS) twiddle.obj timer.obj

kernel.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del kernel.lib $(OBJECTS) $(OBJECTS) 2> nul
