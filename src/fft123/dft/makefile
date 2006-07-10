#
#  dft.lib makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel

!include $(TOPDIR)\..\config\makerules

all: dft.lib $(MDUMMY)

OBJECTS = buffered.obj conf.obj ct_dif.obj ct_dit.obj ct_ditbuf.obj ct_ditf.obj
OBJECTS = $(OBJECTS) ct.obj direct.obj generic.obj indirect.obj kdft_dif.obj
OBJECTS = $(OBJECTS) kdft_difsq.obj kdft_dit.obj kdft.obj nop.obj plan.obj
OBJECTS = $(OBJECTS) problem.obj rader_omega.obj rader.obj rank_geq2.obj
OBJECTS = $(OBJECTS) rank0.obj solve.obj vrank_geq1.obj vrank2_transpose.obj
OBJECTS = $(OBJECTS) vrank3_transpose.obj zero.obj

dft.lib: $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft.lib $(OBJECTS) 2> nul
