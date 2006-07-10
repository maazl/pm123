#
#  rdft_codelets.lib makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft

!include $(TOPDIR)\..\config\makerules

all: rdft_codelets.lib $(MDUMMY)

OBJECTS = hc2r.obj hfb.obj r2hc.obj r2r.obj

rdft_codelets.lib: $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del rdft_codelets.lib $(OBJECTS) 2> nul
