#
#  os2rec.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = os2rec$(CO)
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct$(LBO)
DEF      = os2rec$(LDEF_SUFFX)
#RESOURCE = os2rec$(RES)

all: os2rec.dll os2rec$(LBI) $(MDUMMY)

os2rec.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
#	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

os2rec$(LBI): $(DEF)
	$(LI) $@ os2rec.dll

clean: $(MDUMMY)
	-@del os2rec$(LBI) os2rec.dll $(OBJECTS) $(RESOURCE) 2> nul
