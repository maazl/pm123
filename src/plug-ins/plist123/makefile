#
#  plist123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
TARGET   = plist123.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = plist123$(CO) playlistreader$(CO) playlistwriter$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = plist123$(LDEF_SUFFX)
#RESOURCE = plist123$(RES)
STDLIBS  = $(LIBS_MM)

plist123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
#	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del plist123.dll $(OBJECTS) $(RESOURCE) 2> nul

playlistreader.cpp:	playlistreader.h plist123.h ..\..\utils\fileutil.h ..\..\utils\charset.h ..\..\xio123\xio.h
playlistreader.h:	..\..\include\format.h ..\..\include\decoder_plug.h ..\..\utils\strutils.h ..\..\xio123\xio.h ..\..\utils\cpp/xstring.h ..\..\utils\debuglog.h
..\..\include\format.h:	..\..\include\plugin.h
..\..\include\plugin.h:	..\..\include\config.h ..\..\utils\cpp/xstring.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\utils\cpp/xstring.h:	..\..\utils\cpp/smartptr.h
..\..\utils\cpp/smartptr.h:	..\..\include\config.h ..\..\utils\cpp/cpputil.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp/mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\include\output_plug.h:	..\..\include\format.h
..\..\xio123\xio.h:	..\..\include\config.h
..\..\utils\fileutil.h:	..\..\include\config.h
playlistwriter.cpp:	playlistwriter.h plist123.h ..\..\utils\fileutil.h ..\..\utils\charset.h ..\..\utils\debuglog.h
playlistwriter.h:	..\..\include\decoder_plug.h ..\..\xio123\xio.h ..\..\utils\debuglog.h
plist123.cpp:	..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h plist123.h playlistreader.h playlistwriter.h ..\..\utils\fileutil.h ..\..\xio123\xio.h ..\..\utils\cpp/smartptr.h ..\..\utils\debuglog.h
