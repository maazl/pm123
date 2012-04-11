#
#  wavplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\pulseaudio;$(TOPDIR)\xio123
TARGET   = pulse123.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = pawrapper$(CO) playbackworker$(CO) recordworker$(CO)
OBJECTS  = $(OBJECTS) configuration$(CO) dialog$(CO) pulse123$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\snd123\src\sndfile$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\snd123\src\G72x\g72x$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\snd123\src\GSM610\gsm610$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\libspeex\libspeex$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\pulse\libpulse$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\pulsecore\libpulsecore$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\pulseaudio\json-c\libjson$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF      = pulse123$(LDEF_SUFFX)
RESOURCE = pulse123$(RES)

$(TARGET): $(OBJECTS) $(DEF) $(LIBS) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

pulse123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ $(TARGET)

clean:  $(MDUMMY)
	-@del pulse123.$(LBI) $(TARGET) $(OBJECTS) 2> nul


pulse123.cpp:	pulse123.h playbackworker.h recordworker.h configuration.h dialog.h ..\..\utils\utilfct.h ..\..\include\plugin.h ..\..\utils\debuglog.h
pulse123.h:	..\..\include\plugin.h
..\..\snd123\src\sndfile.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
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
playbackworker.cpp:	playbackworker.h ..\..\utils\cpp\cpputil.h ..\..\utils\debuglog.h
playbackworker.h:	pulse123.h pawrapper.h ..\..\include\output_plug.h ..\..\utils\cpp\smartptr.h ..\..\utils\cpp\mutex.h ..\..\utils\charset.h
pawrapper.h:	..\..\pulseaudio\pulse\thread-mainloop.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\error.h ..\..\pulseaudio\pulse\stream.h ..\..\pulseaudio\pulse\introspect.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\event.h ..\..\utils\cpp\xstring.h ..\..\include\plugin.h ..\..\utils\debuglog.h
..\..\pulseaudio\pulse\thread-mainloop.h:	..\..\pulseaudio\pulse\mainloop-api.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\mainloop-api.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\context.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\mainloop-api.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\operation.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\proplist.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\error.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\stream.h:	..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\format.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\volume.h ..\..\pulseaudio\pulse\def.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\proplist.h
..\..\pulseaudio\pulse\volume.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\version.h
..\..\pulseaudio\pulse\introspect.h:	..\..\pulseaudio\pulse\operation.h ..\..\pulseaudio\pulse\context.h ..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\channelmap.h ..\..\pulseaudio\pulse\volume.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\format.h ..\..\pulseaudio\pulse\version.h
..\..\utils\cpp\event.h:	..\..\include\config.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
dialog.cpp:	dialog.h pulse123.h configuration.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\cpp\pmutils.h ..\..\utils\cpp\algorithm.h ..\..\utils\debuglog.h
dialog.h:	pulse123.h ..\..\include\plugin.h ..\..\include\decoder_plug.h ..\..\utils\cpp\event.h ..\..\utils\cpp\windowbase.h ..\..\utils\debuglog.h
..\..\utils\cpp\windowbase.h:	..\..\utils\cpp\smartptr.h ..\..\utils\cpp\xstring.h ..\..\utils\cpp\pmutils.h ..\..\utils\cpp\container\vector.h ..\..\utils\debuglog.h
..\..\utils\cpp\container\vector.h:	..\..\utils\cpp\mutex.h ..\..\utils\cpp\smartptr.h
..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
..\..\utils\cpp\pmutils.h:	..\..\utils\debuglog.h ..\..\utils\cpp\xstring.h ..\..\include\plugin.h
configuration.cpp:	configuration.h ..\..\utils\inimacro.h ..\..\utils\debuglog.h
configuration.h:	pulse123.h ..\..\utils\cpp\xstring.h
..\..\utils\inimacro.h:	..\..\include\config.h
pawrapper.cpp:	pawrapper.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
recordworker.h:	pulse123.h pawrapper.h ..\..\include\decoder_plug.h ..\..\utils\cpp\smartptr.h ..\..\utils\cpp\mutex.h ..\..\utils\charset.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
recordworker.cpp:	recordworker.h ..\..\utils\cpp\cpputil.h ..\..\utils\debuglog.h
..\..\pulseaudio\pulse\format.h:	..\..\pulseaudio\pulse\cdecl.h ..\..\pulseaudio\pulse\gccmacro.h ..\..\pulseaudio\pulse\proplist.h ..\..\pulseaudio\pulse\sample.h ..\..\pulseaudio\pulse\channelmap.h
..\..\utils\cpp\algorithm.h:	..\..\utils\cpp\container\vector.h
