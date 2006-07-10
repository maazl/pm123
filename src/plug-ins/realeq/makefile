#
#  realeq.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api

!include $(TOPDIR)\config\makerules

OBJECTS  = realeq.obj mmxfir.obj
LIBS     = $(TOPDIR)\utils\utilfct.lib $(TOPDIR)\fft123\fft123.lib
DEF      = realeq$(LDEF_SUFFX)
RESOURCE = realeq.res

all: realeq.dll realeq.lib $(MDUMMY)

realeq.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

realeq.lib: $(DEF)
	$(LI) $@ realeq.dll

clean: $(MDUMMY)
	-@del realeq.lib realeq.dll $(OBJECTS) $(RESOURCE) 2> nul
