#
#  dft_codelets library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = ..\..\kernel;..\..\dft;$(TOPDIR)\include

include $(TOPDIR)\config\makerules

all: dft_codelets$(LBO) $(MDUMMY)

OBJECTS = n$(CO) t$(CO)

dft_codelets$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_codelets$(LBO) $(OBJECTS) 2> nul
