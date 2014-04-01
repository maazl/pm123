#
#  drc123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = drc123.dll
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = DataFile$(CO) Iterators$(CO) FFT2Data$(CO)
OBJECTS  = $(OBJECTS) Filter$(CO) Deconvolution$(CO) OpenLoop$(CO) Calibrate$(CO) Measure$(CO) Generate$(CO)
OBJECTS  = $(OBJECTS) ResponseGraph$(CO) VUMeter$(CO) drc123$(CO)
OBJECTS  = $(OBJECTS) Frontend$(CO) Frontend.OpenLoop$(CO) Frontend.Calibrate$(CO) Frontend.Measure$(CO) Frontend.Generate$(CO) Frontend.Deconvolution$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = drc123$(LDEF_SUFFX)
RESOURCE = drc123$(RES)

drc123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del drc123.dll $(OBJECTS) $(RESOURCE) 2> nul

drc123$(RES):	drc123.h

Calibrate.cpp:	Calibrate.h FFT2Data.h ..\..\utils\debuglog.h
Calibrate.h:	OpenLoop.h DataFile.h DataVector.h ..\..\utils\cpp/mutex.h ..\..\fft123\api\fftw3.h
OpenLoop.h:	Filter.h DataFile.h DataVector.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/event.h ..\..\utils\cpp/smartptr.h ..\..\fft123\api\fftw3.h
Filter.h:	..\..\include\filter_plug.h
DataFile.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/container/sorted_vector.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp/smartptr.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp/cpputil.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
DataVector.h:	..\..\utils\cpp/smartptr.h ..\..\fft123\api\fftw3.h ..\..\utils\debuglog.h
..\..\fft123\api\fftw3.h:	..\..\include\config.h
..\..\utils\cpp\event.h:	..\..\include\config.h ..\..\utils\cpp/mutex.h ..\..\utils\debuglog.h
FFT2Data.h:	OpenLoop.h
DataFile.cpp:	DataFile.h ..\..\utils\cpp/algorithm.h ..\..\utils\debuglog.h
DataVector.cpp:	DataVector.h
Deconvolution.cpp:	Deconvolution.h FFT2Data.h ..\..\utils\strutils.h ..\..\utils\minmax.h
Deconvolution.h:	Filter.h DataVector.h Generate.h ..\..\fft123\api\fftw3.h ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/event.h
drc123.cpp:	drc123.h Frontend.h Deconvolution.h Generate.h Measure.h Calibrate.h ..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h
frontend.h:	drc123.h DataFile.h Iterators.h Deconvolution.h Calibrate.h Measure.h Generate.h ResponseGraph.h VUMeter.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/stringset.h ..\..\utils\cpp/container/sorted_vector.h ..\..\utils\cpp/event.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/windowbase.h ..\..\utils\cpp/dlgcontrols.h
Measure.h:	OpenLoop.h
ResponseGraph.h:	..\..\utils\cpp/windowbase.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/vector.h
..\..\utils\cpp\windowbase.h:	..\..\utils\cpp/smartptr.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/container/vector.h ..\..\utils\vdelegate.h ..\..\utils\debuglog.h
..\..\utils\cpp\pmutils.h:	..\..\utils\debuglog.h ..\..\utils\cpp/xstring.h
..\..\utils\cpp\dlgcontrols.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\debuglog.h
VUMeter.h:	..\..\utils\cpp/windowbase.h
FFT2Data.cpp:	FFT2Data.h Iterators.h ..\..\utils\cpp/algorithm.h ..\..\utils\debuglog.h
..\..\utils\cpp\algorithm.h:	..\..\utils\cpp/container/vector.h
Filter.cpp:	Filter.h Deconvolution.h Measure.h Calibrate.h
Frontend.Calibrate.cpp:	Frontend.h Calibrate.h ..\..\utils\cpp/url123.h
..\..\utils\cpp\url123.h:	..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\cpp/cpputil.h
Frontend.cpp:	Frontend.h Deconvolution.h ..\..\utils\fileutil.h ..\..\include\plugin.h ..\..\utils\cpp/url123.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/directory.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\cpp/container/stringset.h ..\..\utils\debuglog.h
Frontend.Measure.cpp:	Frontend.h Measure.h ..\..\utils\fileutil.h ..\..\utils\cpp/url123.h ..\..\utils\cpp/directory.h
Frontend.OpenLoop.cpp:	Frontend.h Deconvolution.h ..\..\utils\fileutil.h ..\..\include\plugin.h ..\..\utils\cpp/url123.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/directory.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\debuglog.h
Measure.cpp:	Measure.h FFT2Data.h Calibrate.h Deconvolution.h
Reference.cpp:	Reference.h
Reference.h:	..\..\utils\cpp/smartptr.h
ResponseGraph.cpp:	ResponseGraph.h DataFile.h Iterators.h ..\..\utils\cpp/pmutils.h
VUMeter.cpp:	VUMeter.h ..\..\utils\debuglog.h
Generate.h:	DataFile.h Measure.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/container/sorted_vector.h
Frontend.Generate.cpp:	Frontend.h Generate.h ..\..\utils\fileutil.h ..\..\utils\cpp/directory.h ..\..\utils\cpp/url123.h
Generate.cpp:	Generate.h Iterators.h ..\..\utils\cpp/smartptr.h
OpenLoop.cpp:	OpenLoop.h
Frontend.Deconvolution.cpp:	Frontend.h Deconvolution.h FFT2Data.h ..\..\utils\fileutil.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/directory.h ..\..\utils\cpp/container/stringset.h ..\..\utils\debuglog.h
Iterators.h:	DataFile.h ..\..\utils\cpp/cpputil.h
Iterators.cpp:	Iterators.h DataFile.h
