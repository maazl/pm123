#
#  cddaplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\xio123;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = cddaplay$(CO) readcd$(CO) cddb$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
SYSLIBS  = $(LIBS_TCP) $(LIBS_UNI)
DEF      = cddaplay$(LDEF_SUFFX)
RESOURCE = cddaplay$(RES)

all: cddaplay.dll cddaplay$(LBI) $(MDUMMY)

cddaplay.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
  $(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
  $(RC) $(RFLAGS) $(RESOURCE) $@ > nul

cddaplay$(LBI): $(DEF)
  $(LI) $(IFLAGS_OUT)$@ cddaplay.dll

clean:  $(MDUMMY)
	-@del cddaplay$(LBI) cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

cddaplay.c:	..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\utilfct.h ..\..\utils\snprintf.h ..\..\utils\debuglog.h readcd.h cddb.h cddaplay.h
cddb.h:		..\..\xio123\xio.h readcd.h
cddb.c:		cddb.h ..\..\utils\utilfct.h ..\..\utils\snprintf.h ..\..\utils\debuglog.h
readcd.c:	readcd.h ..\..\utils\debuglog.h
cddaplay.h:	..\..\include\format.h
