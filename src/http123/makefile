#
#  http123.dll makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR)\utils

!include $(TOPDIR)\config\makerules

OBJECTS = httpget.obj sockfile.obj
LIBS    = $(LIBS_TCP) $(TOPDIR)\utils\utilfct.lib
DEF     = http$(LDEF_SUFFX)

all: http123.dll http123.lib $(MDUMMY)

http123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

http123.lib: $(DEF)
	$(LI) $@ http123.dll

sockfile.obj: sockfile.c httpget.h sockfile.h
httpget.obj:  httpget.c  httpget.h

clean: $(MDUMMY)
	-@del http123.lib http123.dll $(OBJECTS) 2> nul
