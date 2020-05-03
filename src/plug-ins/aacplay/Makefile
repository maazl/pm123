#
#  aacplay.dll makefile
#

TOPDIR  = ..\\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;$(TOPDIR)\libfaad\include;$(TOPDIR)\libfaad\common\mp4ff;$(TOPDIR)\zlb123
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

include $(TOPDIR)\config\makerules

OBJECTS = aacplay$(CO) mp4decoder$(CO) aacdecoder$(CO) \
          id3v2\id3v2_strings$(CO) id3v2\id3v2_frame$(CO) id3v2\id3v2$(CO)
LIBS    = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) \
        $(TOPDIR)\xio123\xio123$(LBI) $(TOPDIR)\zlb123\zlb123$(LBI) \
        $(TOPDIR)\libfaad\libfaad\libfaad$(LBO) $(TOPDIR)\libfaad\common\mp4ff\mp4ff$(LBO)
SYSLIBS = $(LIBS_UNI)
DEF     = aacplay$(LDEF_SUFFX)

all: aacplay.dll $(MDUMMY)

aacplay.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

aacplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ aacplay.dll

clean:  $(MDUMMY)
	-@del aacplay$(LBI) aacplay.dll $(OBJECTS) 2> nul

mp4decoder.o:	mp4decoder.cpp ..\..\utils\cpp/interlockedptr.h  ..\..\utils\charset.h ..\..\utils\utilfct.h ..\..\libfaad\include\neaacdec.h
aacplay.o:	aacplay.cpp ..\..\include\plugin.h ..\..\utils\fileutil.h mp4decoder.h ..\..\xio123\xio.h ..\..\libfaad\include\neaacdec.h ..\..\utils\cpp/container/vector.h 
aacdecoder.o:	aacdecoder.cpp ..\..\utils\strutils.h 
id3v2\id3v2.o:	id3v2\id3v2.cpp
id3v2\id3v2_frame.o:	id3v2\id3v2_frame.cpp
id3v2\id3v2_strings.o:	id3v2\id3v2_strings.cpp
