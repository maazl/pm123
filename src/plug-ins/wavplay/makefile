#
#  wavplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = wavplay.obj wav.obj
LIBS     = $(TOPDIR)\utils\utilfct.lib
DEF      = wavplay$(LDEF_SUFFX)

all: wavplay.dll wavplay.lib $(MDUMMY)

wavplay.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

wavplay.lib: $(DEF)
	$(LI) $@ wavplay.dll

wavplay.obj: wav.h wavplay.cpp
wav.obj:     wav.h wav.cpp

clean: $(MDUMMY)
	-@del wavplay.lib wavplay.dll $(OBJECTS) 2> nul
