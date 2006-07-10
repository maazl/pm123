#
#  reodft.lib makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft

!include $(TOPDIR)\..\config\makerules

all: reodft.lib $(MDUMMY)

OBJECTS = conf.obj redft00e_r2hc_pad.obj redft00e_r2hc.obj reodft010e_r2hc.obj
OBJECTS = $(OBJECTS) reodft11e_r2hc_odd.obj reodft11e_r2hc.obj
OBJECTS = $(OBJECTS) reodft11e_radix2.obj rodft00e_r2hc_pad.obj
OBJECTS = $(OBJECTS) rodft00e_r2hc.obj

reodft.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del reodft.lib $(OBJECTS) 2> nul
