#
#  wavplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\xio123

!include $(TOPDIR)\config\makerules

OBJECTS  = wavplay$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\snd123\snd123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = wavplay$(LDEF_SUFFX)

all: wavplay.dll wavplay$(LBI) $(MDUMMY)

wavplay.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)

wavplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ wavplay.dll

wavplay$(CO): wavplay.c wavplay.h

clean: $(MDUMMY)
	-@del wavplay$(LBI) wavplay.dll $(OBJECTS) 2> nul
