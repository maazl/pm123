#
#  drc123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = drc123.dll
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = Filter$(CO) Deconvolution$(CO) OpenLoop$(CO) Measure$(CO) Calibrate$(CO)
OBJECTS  = $(OBJECTS) DataFile$(CO) FFT2Data$(CO)
OBJECTS  = $(OBJECTS) ResponseGraph$(CO) VUMeter$(CO) drc123$(CO)
OBJECTS  = $(OBJECTS) Frontend$(CO) Frontend.CalibratePage$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = drc123$(LDEF_SUFFX)
RESOURCE = drc123$(RES)

drc123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del drc123.dll $(OBJECTS) $(RESOURCE) 2> nul

Calibrate.h:	OpenLoop.h DataFile.h DataVector.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/event.h ..\..\fft123\api\fftw3.h
OpenLoop.h:	Filter.h DataFile.h DataVector.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/smartptr.h ..\..\fft123\api\fftw3.h
Filter.h:	..\..\include\filter_plug.h
Deconvolution.cpp:	Deconvolution.h ..\..\utils\strutils.h ..\..\utils\minmax.h
Deconvolution.h:	Filter.h ..\..\fft123\api\fftw3.h ..\..\utils\cpp/smartptr.h
..\..\fft123\api\fftw3.h:	..\..\include\config.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp/cpputil.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
drc123.cpp:	drc123.h frontend.h Deconvolution.h Measure.h Calibrate.h ..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h
frontend.h:	drc123.h DataFile.h Deconvolution.h ResponseGraph.h VUMeter.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/event.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/windowbase.h ..\..\utils\cpp/dlgcontrols.h
..\..\utils\cpp\pmutils.h:	..\..\utils\debuglog.h ..\..\utils\cpp/xstring.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp/smartptr.h
..\..\utils\cpp\windowbase.h:	..\..\utils\cpp/smartptr.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/container/vector.h ..\..\utils\vdelegate.h ..\..\utils\debuglog.h
..\..\utils\cpp\dlgcontrols.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\debuglog.h
Measure.h:	OpenLoop.h
Filter.cpp:	Filter.h Deconvolution.h Measure.h Calibrate.h
frontend.cpp:	frontend.h Deconvolution.h Calibrate.h ..\..\utils\fileutil.h ..\..\include\plugin.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\debuglog.h
Measure.cpp:	Measure.h
Reference.cpp:	Reference.h
DataFile.cpp:	DataFile.h ..\..\utils\debuglog.h
DataFile.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/container/vector.h
ResponseGraph.h:	..\..\utils\cpp/windowbase.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/vector.h
ResponseGraph.cpp:	ResponseGraph.h DataFile.h ..\..\utils\cpp/pmutils.h
VUMeter.cpp:	VUMeter.h ..\..\utils\debuglog.h
Reference.h:	..\..\utils\cpp/smartptr.h
VUMeter.h:	..\..\utils\cpp/windowbase.h
..\..\utils\cpp\event.h:	..\..\include\config.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
FFT2Data.cpp:	FFT2Data.h ..\..\utils\cpp/algorithm.h ..\..\utils\debuglog.h
FFT2Data.h:	DataFile.h DataVector.h
..\..\utils\cpp\algorithm.h:	..\..\utils\cpp/container/vector.h
FreqDomainData.cpp:	FreqDomainData.h ..\..\utils\cpp/cpputil.h ..\..\utils\debuglog.h
Calibrate.cpp:	Calibrate.h FFT2Data.h ..\..\utils\debuglog.h
FreqDomainData.h:	..\..\utils\cpp/smartptr.h ..\..\fft123\api\fftw3.h
DataVector.h:	..\..\utils\cpp/smartptr.h ..\..\fft123\api\fftw3.h ..\..\utils\debuglog.h
DataVector.cpp:	DataVector.h
OpenLoop.cpp:	OpenLoop.h
