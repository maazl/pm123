#
#  analyzer.dll makefile
#

#DEBUG=yes
#DEBUG_ALLOC=yes

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
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del analyzer$(LBI) analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul

analyzer.c:	..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\visual_plug.h ..\..\include\plugin.h analyzer.h colormap.h specana.h ..\..\utils\debuglog.h
specana.h:	..\..\include\format.h
colormap.c:	analyzer.h colormap.h
specana.c:	..\..\include\format.h ..\..\fft123\api\fftw3.h specana.h
