#
#  dft_codelets.lib makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft

!include $(TOPDIR)\..\config\makerules

all: dft_codelets.lib $(MDUMMY)

OBJECTS = n.obj t.obj

dft_codelets.lib: $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_codelets.lib $(OBJECTS) 2> nul
