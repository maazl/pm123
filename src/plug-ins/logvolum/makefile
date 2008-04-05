#
#  logvolum.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = logvolum.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = logvolum$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = logvolum$(LDEF_SUFFX)
RESOURCE = logvolum$(RES)

logvolum.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del logvolum.dll $(OBJECTS) $(RESOURCE) 2> nul
