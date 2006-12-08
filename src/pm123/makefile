#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123;$(TOPDIR)\fft123\api
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) messages$(CO) 123_util$(CO)
OBJECTS  = $(OBJECTS) pfreq$(CO) specana$(CO) plugman$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) docking$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)

all: pm123.exe $(MDUMMY)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe

pm123.c:       pm123.h plugman.h bookmark.h button95.h pfreq.h genre.h docking.h copyright.h messages.h
docking.c:     docking.h
plugman.c:     plugman.h
iniman.c:      pm123.h plugman.h iniman.h
playlist.c:    pm123.h playlist.h
properties.c:  pm123.h plugman.h copyright.h properties.h
bookmark.c:    pm123.h bookmark.h
messages.c:    pm123.h messages.h
skin.c:        pm123.h 
pm123.rc:      pm123.h bookmark.h
pm123.h:       playlist.h skin.h properties.h

clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul
