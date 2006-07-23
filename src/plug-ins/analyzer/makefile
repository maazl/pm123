#
#  analyzer.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = analyzer$(CO)
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct$(LBO)
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer$(RES)

all: analyzer.dll analyzer$(LBI) $(MDUMMY)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

analyzer$(LBI): $(DEF)
	$(LI) $@ analyzer.dll

clean: $(MDUMMY)
	-@del analyzer$(LBI) analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul
