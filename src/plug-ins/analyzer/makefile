#
#  analyzer.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = analyzer$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
SYSLIBS  = $(LIBS_MM)
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer$(RES)

all: analyzer.dll analyzer$(LBI) $(MDUMMY)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

analyzer$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ analyzer.dll

clean:  $(MDUMMY)
	-@del analyzer$(LBI) analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

analyzer.c:	..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\visual_plug.h ..\..\include\plugin.h analyzer.h
