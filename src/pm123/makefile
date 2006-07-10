#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\http123;$(TOPDIR)\fft123\api
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95.obj pm123.obj 123_msg.obj 123_util.obj tag.obj
OBJECTS  = $(OBJECTS) pfreq.obj specana.obj plugman.obj bookmark.obj iniman.obj
OBJECTS  = $(OBJECTS) playlist.obj properties.obj skin.obj
OBJECTS  = $(OBJECTS) docking.obj
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123.res
LIBS     = $(TOPDIR)\gbm123\gbm123.lib $(TOPDIR)\http123\http123.lib
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct.lib $(TOPDIR)\fft123\fft123.lib

all: pm123.exe $(MDUMMY)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe

pm123.c:       pm123.h plugman.h bookmark.h button95.h pfreq.h genre.h docking.h copyright.h
docking.c:     docking.h
iniman.c:      pm123.h plugman.h iniman.h
playlist.c:    pm123.h playlist.h
properties.c:  pm123.h plugman.h copyright.h properties.h
bookmark.c:    pm123.h bookmark.h
skin.c:        pm123.h 
pm123.rc:      pm123.h bookmark.h
pm123.h:       playlist.h skin.h properties.h
playlist.h:    tag.h

clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul
