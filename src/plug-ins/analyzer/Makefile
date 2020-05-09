#
#  analyzer.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = analyzer.dll

include $(TOPDIR)\config\makerules

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

analyzer.o:	analyzer.cpp ..\..\include\format.h ..\..\include\plugin.h analyzer.h specana.h ..\..\include\output_plug.h 
colormap.o:	colormap.cpp colormap.h 
specana.o:	specana.cpp ..\..\include\format.h ..\..\utils\debuglog.h
