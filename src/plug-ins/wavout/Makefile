#
#  wavout.dll makefile
#

TOPDIR  = ..\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;
TARGET  = wavout.dll

include $(TOPDIR)\config\makerules

OBJECTS = writer$(CO) wavout$(CO)
LIBS    = $(TOPDIR)\utils\utilfct$(LBO) \
	$(TOPDIR)\xio123\xio123$(LBI) \
	$(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF     = wavout$(LDEF_SUFFX)
RESOURCE= wavout$(RES)
SYSLIBS = $(LIBS_MM)

all: wavout.dll $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del wavout$(LBI) wavout.dll $(OBJECTS) $(RESOURCE) 2> nul

wavout.o:	wavout.cpp writer.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h  ..\..\utils\cpp/url123.h ..\..\utils\cpp/container/stringmap.h ..\..\xio123\xio.h ..\..\utils\fileutil.h ..\..\include\plugin.h
writer.o:	writer.cpp ..\..\include\output_plug.h 
