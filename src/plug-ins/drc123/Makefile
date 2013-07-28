#
#  drc123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = drc123.dll
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = Filter$(CO) Deconvolution$(CO) OpenLoop$(CO) Measure$(CO) Calibrate$(CO)
OBJECTS  = $(OBJECTS) DataFile$(CO) ResponseGraph$(CO) frontend$(CO) drc123$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = drc123$(LDEF_SUFFX)
RESOURCE = drc123$(RES)

drc123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del drc123.dll $(OBJECTS) $(RESOURCE) 2> nul

Calibrate.cpp:	Calibrate.h
Calibrate.h:	OpenLoop.h
OpenLoop.h:	Filter.h
Filter.h:	..\..\include\filter_plug.h
Deconvolution.cpp:	Deconvolution.h ..\..\utils\strutils.h ..\..\utils\minmax.h
Deconvolution.h:	Filter.h ..\..\fft123\api\fftw3.h ..\..\utils\cpp/smartptr.h
..\..\fft123\api\fftw3.h:	..\..\include\config.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp/cpputil.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
drc123.cpp:	drc123.h frontend.h Deconvolution.h Measure.h Calibrate.h ..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h
frontend.h:	drc123.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/windowbase.h ..\..\utils\cpp/dlgcontrols.h
..\..\utils\cpp\pmutils.h:	..\..\utils\debuglog.h ..\..\utils\cpp/xstring.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp/smartptr.h
..\..\utils\cpp\windowbase.h:	..\..\utils\cpp/smartptr.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/container/vector.h ..\..\utils\debuglog.h
..\..\utils\cpp\dlgcontrols.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\debuglog.h
Measure.h:	OpenLoop.h
Filter.cpp:	Filter.h
frontend.cpp:	frontend.h ..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h
Measure.cpp:	Measure.h
OpenLoop.cpp:	OpenLoop.h
Reference.cpp:	Reference.h
