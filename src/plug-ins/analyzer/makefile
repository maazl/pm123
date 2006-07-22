#
#  analyzer.dll makefile
#

#DEBUG=yes
#DEBUG_ALLOC=yes

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api

!include $(TOPDIR)\config\makerules

OBJECTS  = colormap.obj specana.obj analyzer.obj
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct.lib $(TOPDIR)\fft123\fft123.lib
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer.res

all: analyzer.dll analyzer.lib $(MDUMMY)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

analyzer.lib: $(DEF)
	$(LI) $@ analyzer.dll

clean: $(MDUMMY)
	-@del analyzer.lib analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul
