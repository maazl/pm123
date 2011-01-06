#
#  wavplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\pulseaudio;$(TOPDIR)\xio123
TARGET   = pulse123.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = pulse123$(CO) playbackworker$(CO) pawrapper$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\utils\cpp\cpputil$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\snd123\src\sndfile$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\snd123\src\G72x\g72x$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\snd123\src\GSM610\gsm610$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\libspeex\libspeex$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\pulsecore\libpulsecore$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\pulse\libpulse$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\fft123\fft123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = pulse123$(LDEF_SUFFX)

$(TARGET): $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

pulse123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ $(TARGET)

clean:  $(MDUMMY)
	-@del pulse123.$(LBI) $(TARGET) $(OBJECTS) 2> nul


pulse123.cpp:	pulse123.h playbackworker.h ..\..\include\plugin.h ..\..\utils\debuglog.h
pulse123.h:	..\..\include\output_plug.h
..\..\snd123\src\sndfile.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\include\format.h:	..\..\include\plugin.h
..\..\include\plugin.h:	..\..\include\config.h ..\..\utils\cpp\xstring.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\xio123\xio.h:	..\..\include\config.h
..\..\include\output_plug.h:	..\..\include\format.h
..\..\pulseaudio\pulse\simple.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\sample.h:	..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\version.h:	..\..\pulseaudio\pulse\cdecl.h
..\..\pulseaudio\pulse\channelmap.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\def.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\version.h
playbackworker.cpp:	playbackworker.h ..\..\utils\debuglog.h
playbackworker.h:	pulse123.h pawrapper.h ..\..\utils\cpp\smartptr.h ..\..\utils\charset.h
pawrapper.cpp:	pawrapper.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
pawrapper.h:	..\..\pulseaudio\pulse\thread-mainloop.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\error.h ..\..\pulseaudio\pulse\stream.h ..\..\pulseaudio\pulse\introspect.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\event.h ..\..\include\plugin.h
..\..\pulseaudio\pulse\thread-mainloop.h:	..\..\pulseaudio\pulse\mainloop-api.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\mainloop-api.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\context.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\mainloop-api.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\operation.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\proplist.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\error.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\stream.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\volume.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\proplist.h
..\..\pulseaudio\pulse\volume.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\introspect.h:	..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\volume.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\version.h
..\..\utils\cpp\event.h:	..\..\include\config.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
