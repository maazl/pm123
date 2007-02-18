#
#  wavout.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = wavout$(CO)
LIBS     = $(LIBS_MM) $(TOPDIR)\utils\utilfct$(LBO)
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout$(RES)

all: wavout.dll wavout$(LBI) $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

wavout$(LBI): $(DEF)
	$(LI) $@ wavout.dll

clean: $(MDUMMY)
	-@del wavout$(LBI) wavout.dll $(OBJECTS) $(RESOURCE) 2> nul
