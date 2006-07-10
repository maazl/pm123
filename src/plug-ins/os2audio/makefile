#
#  os2audio.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = os2audio.obj
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct.lib
DEF      = os2audio$(LDEF_SUFFX)
RESOURCE = os2audio.res

all: os2audio.dll os2audio.lib $(MDUMMY)

os2audio.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

os2audio.lib: $(DEF)
	$(LI) $@ os2audio.dll

clean: $(MDUMMY)
	-@del os2audio.lib os2audio.dll $(OBJECTS) $(RESOURCE) 2> nul
