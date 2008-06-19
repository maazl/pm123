#
#  realeq.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api

!include $(TOPDIR)\config\makerules

OBJECTS  = realeq$(CO) mmxfir$(OBJ)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = realeq$(LDEF_SUFFX)
RESOURCE = realeq$(RES)

all: realeq.dll realeq$(LBI) $(MDUMMY)

realeq.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

realeq$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ realeq.dll

clean:  $(MDUMMY)
	-@del realeq$(LBI) realeq.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

realeq.c:	..\..\utils\utilfct.h ..\..\fft123\api\fftw3.h ..\..\include\format.h ..\..\include\filter_plug.h ..\..\include\output_plug.h ..\..\include\decoder_plug.h ..\..\include\plugin.h realeq.h
