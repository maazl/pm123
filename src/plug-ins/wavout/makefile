#
#  wavout.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = wavout.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = wavout$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
DEF      = wavout$(LDEF_SUFFX)
RESOURCE = wavout$(RES)
SYSLIBS  = $(LIBS_MM)

all: wavout.dll $(MDUMMY)

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del wavout$(LBI) wavout.dll $(OBJECTS) $(RESOURCE) 2> nul

wavout.c:	wavout.h
