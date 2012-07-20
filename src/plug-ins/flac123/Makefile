#
#  flac123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
INCDIR   = $(INCDIR);$(TOPDIR)\libflac\include
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = decoder$(CO) flac123$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\libflac\src\libFLAC\libFLAC$(LBO) $(TOPDIR)\libflac\src\libFLAC++\libFLAC++$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\ogg123\src\libogg$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
SYSLIBS  = $(LIBS_UNI)
DEF      = flac123$(LDEF_SUFFX)

all: flac123.dll $(MDUMMY)

flac123.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

flac123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ flac123.dll

clean:  $(MDUMMY)
	-@del flac123$(LBI) flac123.dll $(OBJECTS) 2> nul

decoder.cpp:	decoder.h ..\..\utils\cpp\container\stringmap.h ..\..\utils\charset.h ..\..\utils\debuglog.h
decoder.h:	..\..\libflac\include\FLAC\stream_decoder.h ..\..\include\decoder_plug.h ..\..\utils\cpp\xstring.h ..\..\utils\cpp\atomic.h ..\..\utils\cpp\mutex.h ..\..\xio123\xio.h
..\..\libflac\include\FLAC\stream_decoder.h:	..\..\include\format.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\include\output_plug.h:	..\..\include\format.h
..\..\utils\cpp\atomic.h:	..\..\include\config.h ..\..\utils\interlocked.h
..\..\xio123\xio.h:	..\..\include\config.h
..\..\utils\cpp\container\stringmap.h:	..\..\utils\cpp\xstring.h ..\..\utils\cpp\container\sorted_vector.h
..\..\utils\cpp\container\sorted_vector.h:	..\..\utils\cpp\smartptr.h ..\..\utils\cpp\algorithm.h ..\..\utils\cpp\container\vector.h
..\..\utils\cpp\algorithm.h:	..\..\utils\cpp\container\vector.h
..\..\utils\cpp\container\vector.h:	..\..\utils\cpp\mutex.h ..\..\utils\cpp\smartptr.h
flac123.cpp:	..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h ..\..\utils\charset.h ..\..\utils\debuglog.h decoder.h
..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
..\..\utils\fileutil.h:	..\..\include\config.h
