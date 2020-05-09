#
#  r2r library makefile
#

DLL     = yes
TOPDIR  = ..\..\..\..
INCDIR  = ..\..\..\kernel;..\..\..\rdft;..\..\..\rdft\codelets;$(TOPDIR)\include

include $(TOPDIR)\config\makerules

all: r2r$(LBO) $(MDUMMY)

OBJECTS = r2r_codlist$(CO) e01_8$(CO) e10_8$(CO)

r2r$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del r2r$(LBO) $(OBJECTS) 2> nul
