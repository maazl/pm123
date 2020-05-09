#
#  realeq.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = realeq.dll

include $(TOPDIR)\config\makerules

OBJECTS  = realeq$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = realeq$(LDEF_SUFFX)
RESOURCE = realeq$(RES)

realeq.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del realeq$(LBI) realeq.dll $(OBJECTS) $(RESOURCE) 2> nul

realeq.rc:	realeq.h
realeq.o:	realeq.cpp ..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\plugin.h ..\..\utils\debuglog.h
