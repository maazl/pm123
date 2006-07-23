#
#  wavplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = wavplay$(CO) wav$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = wavplay$(LDEF_SUFFX)

all: wavplay.dll wavplay$(LBI) $(MDUMMY)

wavplay.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

wavplay$(LBI): $(DEF)
	$(LI) $@ wavplay.dll

wavplay$(CO): wav.h wavplay.cpp
wav$(CO):     wav.h wav.cpp

clean: $(MDUMMY)
	-@del wavplay$(LBI) wavplay.dll $(OBJECTS) 2> nul
