#
#  oggplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\vrb123\include;$(TOPDIR)\xio123
INCDIR   = $(INCDIR);$(TOPDIR)\ogg123\include
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = vcedit$(CO) oggdecoder$(CO) oggplay$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\vrb123\lib\libvorbis$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\ogg123\src\libogg$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
SYSLIBS  = $(LIBS_UNI)
DEF      = oggplay$(LDEF_SUFFX)

all: oggplay.dll $(MDUMMY)

oggplay.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

oggplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ oggplay.dll

clean:  $(MDUMMY)
	-@del oggplay$(LBI) oggplay.dll $(OBJECTS) 2> nul

oggplay.cpp:	..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h ..\..\utils\charset.h ..\..\utils\debuglog.h oggdecoder.h vcedit.h
..\..\vrb123\include\vorbis\vorbisfile.h:	..\..\vrb123\include\vorbis\codec.h
..\..\vrb123\include\vorbis\codec.h:	..\..\ogg123\include\ogg\ogg.h
..\..\ogg123\include\ogg\ogg.h:	..\..\ogg123\include\ogg\os_types.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h ..\..\include\plugin.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\output_plug.h:	..\..\include\format.h
..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
..\..\utils\fileutil.h:	..\..\include\config.h
oggplay.h:	..\..\include\format.h ..\..\xio123\xio.h ..\..\ogg123\include\ogg\ogg.h
..\..\xio123\xio.h:	..\..\include\config.h
vcedit.h:	..\..\ogg123\include\ogg\ogg.h ..\..\vrb123\include\vorbis\codec.h
vcedit.cpp:	..\..\ogg123\include\ogg\ogg.h ..\..\vrb123\include\vorbis\codec.h vcedit.h
oggdecoder.cpp:	oggdecoder.h ..\..\utils\cpp\interlockedptr.h ..\..\utils\charset.h ..\..\utils\eautils.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h
oggdecoder.h:	..\..\include\decoder_plug.h ..\..\xio123\xio.h ..\..\ogg123\include\ogg\ogg.h ..\..\vrb123\include\vorbis\vorbisfile.h ..\..\utils\cpp\mutex.h ..\..\utils\cpp\container\vector.h
..\..\utils\cpp\container\vector.h:	..\..\utils\cpp\mutex.h ..\..\utils\cpp\smartptr.h
..\..\utils\cpp\interlockedptr.h:	..\..\utils\interlocked.h
