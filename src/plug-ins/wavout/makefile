#
#  wavout.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = wavout.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = wavout$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout$(RES)
STDLIBS  = $(LIBS_MM)

wavout.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del wavout.dll $(OBJECTS) $(RESOURCE) 2> nul
