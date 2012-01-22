#
#  wavplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\xio123
TARGET   = wavplay.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = wavplay$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\snd123\src\sndfile$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\snd123\src\G72x\g72x$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\snd123\src\GSM610\gsm610$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = wavplay$(LDEF_SUFFX)

wavplay.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

wavplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ wavplay.dll

clean:  $(MDUMMY)
	-@del wavplay$(LBI) wavplay.dll $(OBJECTS) 2> nul

wavplay.c:	wavplay.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\snprintf.h ..\..\utils\debuglog.h
wavplay.h:	..\..\snd123\src\sndfile.h ..\..\include\format.h ..\..\xio123\xio.h
..\..\snd123\src\sndfile.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h ..\..\include\plugin.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\xio123\xio.h:	..\..\include\config.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
