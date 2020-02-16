#
#  mp4play.dll makefile
#

TOPDIR  = ..\\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;$(TOPDIR)\libfaad\include;$(TOPDIR)\libfaad\common\mp4ff
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

include $(TOPDIR)\config\makerules

OBJECTS = mp4decoder$(CO) mp4play$(CO)
LIBS    = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI) \
	$(TOPDIR)\libfaad\libfaad\libfaad$(LBO) $(TOPDIR)\libfaad\common\mp4ff\mp4ff$(LBO)
SYSLIBS = $(LIBS_UNI)
DEF     = mp4play$(LDEF_SUFFX)

all: mp4play.dll $(MDUMMY)

mp4play.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

mp4play$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ mp4play.dll

clean:  $(MDUMMY)
	-@del mp4play$(LBI) mp4play.dll $(OBJECTS) 2> nul

mp4play.cpp:	..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h ..\..\utils\charset.h ..\..\utils\debuglog.h mp4decoder.h
mp4decoder.cpp:	mp4decoder.h ..\..\utils\cpp/interlockedptr.h ..\..\utils\strutils.h ..\..\utils\charset.h ..\..\utils\eautils.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h
mp4decoder.h:	..\..\include\decoder_plug.h ..\..\xio123\xio.h ..\..\libfaad\common\mp4ff\mp4ff.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/container/vector.h
..\..\libfaad\common\mp4ff\mp4ff.h:	..\..\libfaad\common\mp4ff\mp4ff_int_types.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\cpp\interlockedptr.h:	..\..\utils\interlocked.h
