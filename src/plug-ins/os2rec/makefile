#
#  os2rec.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = os2rec.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = os2rec$(CO)
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct$(LBO)
DEF      = os2rec$(LDEF_SUFFX)
RESOURCE = os2rec$(RES)

os2rec.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del os2rec.dll $(OBJECTS) $(RESOURCE) 2> nul
os2rec.cpp:	..\..\utils\utilfct.h ..\..\utils\errorstr.h ..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h os2rec.h ..\..\utils\debuglog.h
