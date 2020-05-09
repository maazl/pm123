#
#  drc123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = drc123.dll
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

include $(TOPDIR)\config\makerules

OBJECTS = DataFile$(CO) Iterators$(CO) FFT2Data$(CO) \
	Filter$(CO) Deconvolution$(CO) OpenLoop$(CO) Calibrate$(CO) Measure$(CO) Generate$(CO) \
	ResponseGraph$(CO) VUMeter$(CO) drc123$(CO) \
	Frontend$(CO) Frontend.OpenLoop$(CO) Frontend.Calibrate$(CO) Frontend.Measure$(CO) Frontend.Generate$(CO) Frontend.Deconvolution$(CO)
LIBS    = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF     = drc123$(LDEF_SUFFX)
RESOURCE= drc123$(RES)

drc123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del drc123.dll $(OBJECTS) $(RESOURCE) 2> nul

drc123$(RES):	drc123.h

Calibrate.o:	Calibrate.cpp FFT2Data.h 
DataFile.o:	DataFile.cpp ..\..\utils\cpp/algorithm.h 
Deconvolution.o:	Deconvolution.cpp FFT2Data.h  ..\..\utils\minmax.h
FFT2Data.o:	FFT2Data.cpp Iterators.h ..\..\utils\cpp/cpputil.h ..\..\utils\debuglog.h
Filter.o:	Filter.cpp Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h  ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h Calibrate.h DataFile.h ..\..\utils\cpp/container/sorted_vector.h
Frontend.Calibrate.o:	Frontend.Calibrate.cpp Calibrate.h DataFile.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h  ..\..\utils\cpp/container/sorted_vector.h
Frontend.Deconvolution.o:	Frontend.Deconvolution.cpp Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h  ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h ..\..\utils\fileutil.h ..\..\utils\cpp/directory.h ..\..\utils\debuglog.h
Frontend.Generate.o:	Frontend.Generate.cpp Generate.h Measure.h  ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/cpputil.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h ..\..\utils\cpp/directory.h
Frontend.Measure.o:	Frontend.Measure.cpp Measure.h  ..\..\utils\cpp/url123.h ..\..\utils\cpp/container/stringmap.h
Frontend.OpenLoop.o:	Frontend.OpenLoop.cpp Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h  ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h ..\..\include\plugin.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/stringmap.h
Frontend.o:	Frontend.cpp Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h  ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h ..\..\include\plugin.h ..\..\utils\cpp/dlgcontrols.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\debuglog.h
Generate.o:	Generate.cpp Iterators.h ..\..\utils\cpp/cpputil.h 
Iterators.o:	Iterators.cpp DataFile.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h  ..\..\utils\cpp/container/sorted_vector.h
Measure.o:	Measure.cpp FFT2Data.h  Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h ..\..\utils\cpp/event.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h
OpenLoop.o:	OpenLoop.cpp 
ResponseGraph.o:	ResponseGraph.cpp DataFile.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h  ..\..\utils\cpp/container/sorted_vector.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/xstring.h
VUMeter.o:	VUMeter.cpp ..\..\utils\debuglog.h
drc123.o:	drc123.cpp Frontend.h DataFile.h ..\..\utils\cpp/mutex.h ..\..\utils\interlocked.h  ..\..\utils\cpp/container/sorted_vector.h Deconvolution.h DataVector.h ..\..\fft123\api\fftw3.h ..\..\utils\cpp/event.h Measure.h ResponseGraph.h ..\..\utils\cpp/container/vector.h ..\..\utils\cpp/xstring.h ..\..\utils\cpp/pmutils.h ..\..\utils\cpp/dlgcontrols.h Generate.h ..\..\utils\cpp/smartptr.h ..\..\utils\cpp/cpputil.h Calibrate.h ..\..\include\filter_plug.h ..\..\utils\debuglog.h
