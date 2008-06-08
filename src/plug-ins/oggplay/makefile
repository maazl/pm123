#
#  oggplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\vrb123\include;$(TOPDIR)\xio123
INCDIR   = $(INCDIR);$(TOPDIR)\ogg123\include

!include $(TOPDIR)\config\makerules

OBJECTS  = oggplay$(CO) vcedit$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\vrb123\vrb123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\ogg123\ogg123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = oggplay$(LDEF_SUFFX)
STDLIBS  = $(LIBS_UNI)

all: oggplay.dll oggplay$(LBI) $(MDUMMY)

oggplay.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)

oggplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ oggplay.dll

oggplay$(CO): oggplay.c oggplay.h vcedit.h
vcedit$(CO):  vcedit.c  vcedit.h

clean: $(MDUMMY)
	-@del oggplay$(LBI) oggplay.dll $(OBJECTS) 2> nul
oggplay.c:	..\..\vrb123\include\vorbis/vorbisfile.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\snprintf.h ..\..\utils\debuglog.h oggplay.h vcedit.h
vcedit.h:	..\..\ogg123\include\ogg/ogg.h ..\..\vrb123\include\vorbis/codec.h
vcedit.c:	..\..\ogg123\include\ogg/ogg.h ..\..\vrb123\include\vorbis/codec.h vcedit.h
