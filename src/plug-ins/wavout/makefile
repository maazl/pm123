#
#  wavout.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;
TARGET   = wavout.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = writer$(CO) wavout$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout$(RES)
SYSLIBS  = $(LIBS_MM)

all: wavout.dll $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del wavout$(LBI) wavout.dll $(OBJECTS) $(RESOURCE) 2> nul

wavout.cpp:	..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\output_plug.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h
