#
#  realeq.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = realeq.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = realeq$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = realeq$(LDEF_SUFFX)
RESOURCE = realeq$(RES)

realeq.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del realeq$(LBI) realeq.dll $(OBJECTS) $(RESOURCE) 2> nul

realeq.c:	..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\plugin.h realeq.h ..\..\utils\debuglog.h
