#
#  logvolum.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = logvolum$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = logvolum$(LDEF_SUFFX)
#RESOURCE = logvolum$(RES)

all: logvolum.dll logvolum$(LBI) $(MDUMMY)

logvolum.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
#	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

logvolum$(LBI): $(DEF)
	$(LI) $@ logvolum.dll

clean: $(MDUMMY)
	-@del logvolum$(LBI) logvolum.dll $(OBJECTS) $(RESOURCE) 2> nul
