#
#  rdft_codelets library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = ..\..\kernel;..\..\rdft;$(TOPDIR)\include

include $(TOPDIR)\config\makerules

all: rdft_codelets$(LBO) $(MDUMMY)

OBJECTS = hc2r$(CO) hfb$(CO) r2hc$(CO) r2r$(CO)

rdft_codelets$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del rdft_codelets$(LBO) $(OBJECTS) 2> nul
