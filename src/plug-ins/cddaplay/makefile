#
#  cddaplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\xio123;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = cddaplay$(CO) readcd$(CO) cddb$(CO)
LIBS     = $(LIBS_TCP) $(LIBS_UNI) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = cddaplay$(LDEF_SUFFX)
RESOURCE = cddaplay$(RES)

all: cddaplay.dll cddaplay$(LBI) $(MDUMMY)

cddaplay.dll: $(OBJECTS) $(DEF) $(RESOURCE)
  $(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
  $(RC) $(RFLAGS) $(RESOURCE) $@ > nul

cddaplay$(LBI): $(DEF)
  $(LI) $@ cddaplay.dll

cddb$(CO):     cddb.c   cddb.h
readcd$(CO):   readcd.c readcd.h
cddaplay$(CO): cddaplay.c cddaplay.h readcd.h cddb.h

clean: $(MDUMMY)
  -@del cddaplay$(LBI) cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
