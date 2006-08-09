#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\http123
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) 123_msg$(CO) 123_util$(CO) tag$(CO)
OBJECTS  = $(OBJECTS) pfreq$(CO) plugman$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) docking$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\http123\http123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO)

all: pm123.exe $(MDUMMY)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe

123_msg.c:     pm123.h plugman.h
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
