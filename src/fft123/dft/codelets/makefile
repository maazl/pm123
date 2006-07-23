#
#  dft_codelets library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft

!include $(TOPDIR)\..\config\makerules

all: dft_codelets$(LBO) $(MDUMMY)

OBJECTS = n$(CO) t$(CO)

dft_codelets$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_codelets$(LBO) $(OBJECTS) 2> nul
