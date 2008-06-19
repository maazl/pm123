#
#  os2audio.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS  = os2audio$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO)
SYSLIBS  = $(LIBS_MM)
DEF      = os2audio$(LDEF_SUFFX)
RESOURCE = os2audio$(RES)

all: os2audio.dll os2audio$(LBI) $(MDUMMY)

os2audio.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

os2audio$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ os2audio.dll

clean:  $(MDUMMY)
	-@del os2audio$(LBI) os2audio.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

os2audio.c:	..\..\utils\utilfct.h ..\..\include\format.h ..\..\include\output_plug.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\debuglog.h os2audio.h
