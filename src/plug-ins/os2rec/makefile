#
#  os2rec.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = os2rec.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = os2rec$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = os2rec$(LDEF_SUFFX)
RESOURCE = os2rec$(RES)
STDLIBS  = $(LIBS_MM)

os2rec.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del os2rec.dll $(OBJECTS) $(RESOURCE) 2> nul
os2rec.cpp:	..\..\utils\utilfct.h ..\..\utils\errorstr.h ..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h os2rec.h ..\..\utils\debuglog.h
