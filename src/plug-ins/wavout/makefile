#
#  wavout.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = wavout$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout$(RES)

all: wavout.dll wavout$(LBI) $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

wavout$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ wavout.dll

clean:  $(MDUMMY)
	-@del wavout$(LBI) wavout.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -q -I$(INCDIR)

wavout.c:	wavout.h
