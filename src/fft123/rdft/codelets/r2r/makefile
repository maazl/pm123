#
#  r2r.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft;$(TOPDIR)\rdft\codelets

!include $(TOPDIR)\..\config\makerules

all: r2r.lib $(MDUMMY)

OBJECTS = r2r_codlist.obj e01_8.obj e10_8.obj

r2r.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del r2r.lib $(OBJECTS) 2> nul
