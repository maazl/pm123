#
#  os2audio.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = os2audio$(CO)
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct$(LBO)
DEF      = os2audio$(LDEF_SUFFX)
RESOURCE = os2audio$(RES)

all: os2audio.dll os2audio$(LBI) $(MDUMMY)

os2audio.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

os2audio$(LBI): $(DEF)
	$(LI) $@ os2audio.dll

os2audio$(CO): os2audio.c os2audio.h

clean: $(MDUMMY)
	-@del os2audio$(LBI) os2audio.dll $(OBJECTS) $(RESOURCE) 2> nul
