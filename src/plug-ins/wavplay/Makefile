#
#  wavplay.dll makefile
#

TOPDIR  = ..\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\xio123
TARGET  = wavplay.dll

include $(TOPDIR)\config\makerules

OBJECTS = wavplay$(CO)
LIBS    = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\snd123\src\sndfile$(LBO) \
	$(TOPDIR)\snd123\src\G72x\g72x$(LBO) \
	$(TOPDIR)\snd123\src\GSM610\gsm610$(LBO) \
	$(TOPDIR)\xio123\xio123$(LBI) \
	$(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF     = wavplay$(LDEF_SUFFX)

wavplay.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

wavplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ wavplay.dll

clean:  $(MDUMMY)
	-@del wavplay$(LBI) wavplay.dll $(OBJECTS) 2> nul

wavplay.o:	wavplay.cpp ..\..\include\plugin.h ..\..\utils\snprintf.h 
