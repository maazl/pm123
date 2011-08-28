#
#  mpg123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;$(TOPDIR)\libmpg123;$(TOPDIR)\zlb123;
TARGET   = mpg123.dll
SRCFIL   = *.c* id3v1\*.c* id3v2\*.c*

!include $(TOPDIR)\config\makerules

#OBJECTS  = dct64_MMX$(OBJ) decode_MMX$(OBJ) tabinit_MMX$(OBJ)
#OBJECTS  = $(OBJECTS) mpg123$(CO) common$(CO) decode_i386$(CO)
#OBJECTS  = $(OBJECTS) layer1$(CO) layer2$(CO) layer3$(CO)
#OBJECTS  = $(OBJECTS) dxhead$(CO) equalizer$(CO) dialog$(CO)
OBJECTS  = mpg123$(CO) dialog$(CO)
#OBJECTS  = $(OBJECTS) id3v1\genre$(CO)
OBJECTS  = $(OBJECTS) id3v1\id3v1$(CO) id3v1\genre$(CO)
OBJECTS  = $(OBJECTS) id3v2\id3v2_strings$(CO) id3v2\id3v2_frame$(CO) id3v2\id3v2$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI) $(TOPDIR)\libmpg123\libmpg123$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\zlb123\zlb123$(LBI) $(TOPDIR)\utils\cpp\cpputil$(LBO)
SYSLIBS  = $(LIBS_UNI)
DEF      = mpg123$(LDEF_SUFFX)
RESOURCE = mpg123$(RES)
STDLIBS  = $(LIBS_UNI)


mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

mpg123$(RES): mpg123.rc mpg123.h

clean: $(MDUMMY)
	-@del mpg123$(LBI) mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul

dialog.cpp:	dialog.h mpg123.h ..\..\utils\charset.h ..\..\utils\utilfct.h ..\..\utils\fileutil.h ..\..\include\decoder_plug.h ..\..\utils\debuglog.h ..\..\utils\snprintf.h ..\..\utils\eautils.h
..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\include\plugin.h:	..\..\include\config.h ..\..\utils\cpp\xstring.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\include\format.h:	..\..\include\plugin.h
..\..\include\output_plug.h:	..\..\include\format.h
..\..\xio123\xio.h:	..\..\include\config.h
id3v1\id3v1.h:	..\..\xio123\xio.h
id3v2\id3v2.h:	..\..\xio123\xio.h
..\..\utils\cpp\container\vector.h:	..\..\utils\cpp\mutex.h ..\..\utils\cpp\smartptr.h ..\..\utils\cpp\xstring.h
..\..\utils\fileutil.h:	..\..\include\config.h
common.c:	tables.h ..\..\utils\debuglog.h ..\..\utils\strutils.h ..\..\utils\minmax.h
dxhead.c:	dxhead.h
layer3.c:	..\..\utils\debuglog.h huffman.h
..\..\libmpg123\config.h:	..\..\include\config.h
mpg123.h:	..\..\utils\utilfct.h ..\..\include\plugin.h ..\..\include\decoder_plug.h ..\..\xio123\xio.h ..\..\libmpg123\config.h ..\..\libmpg123\mpg123.h ..\..\utils\cpp\mutex.h ..\..\utils\cpp\container\vector.h
mpg123.cpp:	mpg123.h dialog.h ..\..\utils\charset.h ..\..\include\decoder_plug.h ..\..\utils\debuglog.h ..\..\utils\snprintf.h ..\..\utils\eautils.h ..\..\utils\fileutil.h
