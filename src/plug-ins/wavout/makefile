#
#  wavout.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = wavout.obj ..\wavplay\wav.obj
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct.lib
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout.res

all: wavout.dll wavout.lib $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

wavout.lib: $(DEF)
	$(LI) $@ wavout.dll

clean: $(MDUMMY)
	-@del wavout.lib wavout.dll $(OBJECTS) $(RESOURCE) 2> nul
