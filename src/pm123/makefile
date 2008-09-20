#
#  pm123.dll makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123;$(TOPDIR)\fft123\api
DLL      = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) messages$(CO)
OBJECTS  = $(OBJECTS) pfreq$(CO) specana$(CO) plugman$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) properties$(CO) skin$(CO) tags$(CO)
OBJECTS  = $(OBJECTS) docking$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
SYSLIBS  = $(LIBS_UNI)

all: pm123.dll $(MDUMMY)

pm123.dll: $(OBJECTS) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

clean: $(MDUMMY)
	-@del pm123.dll $(OBJECTS) $(RESOURCE) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

bookmark.c:	..\utils\utilfct.h pm123.h bookmark.h docking.h iniman.h assertions.h playlist.h
pm123.h:	..\include\decoder_plug.h skin.h properties.h copyright.h plugman.h
button95.c:	button95.h ..\utils\utilfct.h
docking.c:	pm123.h docking.h ..\utils\utilfct.h
iniman.c:	..\utils\snprintf.h ..\utils\utilfct.h pm123.h iniman.h plugman.h
messages.c:	..\utils\debuglog.h ..\utils\utilfct.h pm123.h plugman.h messages.h assertions.h
pfreq.c:	..\utils\utilfct.h ..\utils\snprintf.h ..\utils\debuglog.h pm123.h pfreq.h docking.h playlist.h assertions.h iniman.h
playlist.c:	..\utils\utilfct.h playlist.h pm123.h plugman.h docking.h iniman.h pfreq.h assertions.h tags.h
plugman.c:	..\utils\utilfct.h ..\utils\debuglog.h ..\utils\snprintf.h pm123.h plugman.h
pm123.c:	..\utils\utilfct.h ..\utils\snprintf.h ..\utils\debuglog.h pm123.h bookmark.h button95.h pfreq.h docking.h iniman.h messages.h assertions.h playlist.h tags.h
properties.c:	..\utils\snprintf.h properties.h pm123.h plugman.h iniman.h
skin.c:		..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h skin.h pm123.h plugman.h playlist.h pfreq.h bookmark.h button95.h
specana.c:	..\utils\utilfct.h ..\fft123\api\fftw3.h plugman.h
tags.c:		..\utils\snprintf.h ..\include\decoder_plug.h ..\utils\queue.h pm123.h plugman.h assertions.h genre.h playlist.h tags.h
properties.h:	..\xio123\xio.h
plugman.h:	..\include\format.h ..\include\decoder_plug.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\visual_plug.h ..\include\plugin.h ..\utils\utilfct.h
playlist.h:	..\include\format.h ..\include\decoder_plug.h
