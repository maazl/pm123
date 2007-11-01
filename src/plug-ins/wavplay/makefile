#
#  wavplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\xio123
TARGET   = wavplay.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = wavplay$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\snd123\snd123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = wavplay$(LDEF_SUFFX)

wavplay.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

wavplay$(CO): wavplay.c wavplay.h

clean: $(MDUMMY)
	-@del wavplay.dll $(OBJECTS) 2> nul
