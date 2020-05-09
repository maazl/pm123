#
#  wavplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\snd123\src;$(TOPDIR)\pulseaudio;$(TOPDIR)\xio123
TARGET   = pulse123.dll

include $(TOPDIR)\config\makerules

OBJECTS = pawrapper$(CO) playbackworker$(CO) recordworker$(CO) \
	configuration$(CO) dialog$(CO) pulse123$(CO)
LIBS    = $(TOPDIR)\utils\utilfct$(LBO) \
	$(TOPDIR)\pulseaudio\pulse\libpulse$(LBO) \
	$(TOPDIR)\pulseaudio\pulsecore\libpulsecore$(LBO) \
	$(TOPDIR)\pulseaudio\json-c\libjson$(LBO) \
	$(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF      = pulse123$(LDEF_SUFFX)
RESOURCE = pulse123$(RES)

pulse123$(LBO) : $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

$(TARGET): pulse123$(LBO) $(DEF) $(LIBS) $(RESOURCE)
#	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LLIB_PREFX)pulse123$(LBO) $(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

pulse123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ $(TARGET)

clean:  $(MDUMMY)
	-@del pulse123.$(LBI) $(TARGET) $(OBJECTS) 2> nul


configuration.o:	configuration.cpp ..\..\utils\inimacro.h 
dialog.o:	dialog.cpp pulse123.h  ..\..\include\plugin.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/algorithm.h
pawrapper.o:	pawrapper.cpp ..\..\utils\interlocked.h 
playbackworker.o:	playbackworker.cpp ..\..\utils\cpp/cpputil.h 
pulse123.o:	pulse123.cpp playbackworker.h pawrapper.h ..\..\pulseaudio\pulse/context.h ..\..\pulseaudio\pulse/def.h ..\..\pulseaudio\pulse/sample.h ..\..\pulseaudio\pulse/cdecl.h  ..\..\pulseaudio\pulse/proplist.h ..\..\pulseaudio\pulse/gccmacro.h ..\..\pulseaudio\pulse/error.h ..\..\pulseaudio\pulse/version.h ..\..\pulseaudio\pulse/introspect.h ..\..\pulseaudio\pulse/volume.h ..\..\pulseaudio\pulse/channelmap.h ..\..\pulseaudio\pulse/format.h ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h ..\..\include\plugin.h ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/cpputil.h ..\..\utils\charset.h configuration.h ..\..\utils\cpp/xstring.h ..\..\utils\utilfct.h ..\..\utils\debuglog.h
recordworker.o:	recordworker.cpp ..\..\utils\cpp/cpputil.h 
