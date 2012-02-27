#
#  analyzer.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = analyzer.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = colormap$(CO) specana$(CO) analyzer$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
SYSLIBS  = $(LIBS_MM)
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer$(RES)

all: analyzer.dll $(MDUMMY)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del analyzer$(LBI) analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul

..\..\utils\utilfct.h:	..\..\include\config.h ..\..\utils\errorstr.h ..\..\utils\minmax.h ..\..\utils\strutils.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h ..\..\include\plugin.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\visual_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
specana.h:	..\..\include\format.h ..\..\include\output_plug.h
colormap.cpp:	analyzer.h colormap.h ..\..\utils\debuglog.h
..\..\fft123\api\fftw3.h:	..\..\include\config.h
analyzer.cpp:	..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\visual_plug.h ..\..\include\plugin.h ..\..\fft123\api\fftw3.h analyzer.h colormap.h specana.h ..\..\utils\debuglog.h
..\..\include\output_plug.h:	..\..\include\format.h
specana.cpp:	specana.h ..\..\include\format.h ..\..\fft123\api\fftw3.h ..\..\utils\debuglog.h
