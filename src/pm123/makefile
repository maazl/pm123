#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) messages$(CO) 123_util$(CO)
OBJECTS  = $(OBJECTS) pfreq$(CO) plugman$(CO) plugman_base$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) docking$(CO) vdelegate$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO)
#STDLIBS  = $(LIBS_UNI)

all: pm123.exe $(MDUMMY)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

plugman$(CO):	plugman.cpp
plugman_base$(CO):	plugman_base.cpp

123_util.c:    pm123.h plugman.h
plugman.cpp:   pm123.h plugman.h plugman_base.h
plugman_base.cpp: plugman.h plugman_base.h
pm123.c:       pm123.h plugman.h bookmark.h button95.h pfreq.h docking.h copyright.h messages.h
docking.c:     docking.h
iniman.c:      pm123.h plugman.h iniman.h
playlist.c:    pm123.h playlist.h
properties.c:  pm123.h plugman.h copyright.h properties.h
bookmark.c:    pm123.h bookmark.h
skin.c:        pm123.h plugman.h
vdelegate.c:   vdelegate.h
pipe.c:        playlist.h pm123.h properties.h
messages.c:    pm123.h messages.h
pm123.rc:      pm123.h bookmark.h
pm123.h:       playlist.h skin.h properties.h
playable.cpp:  playable.h
event.cpp:     event.h
mutex.cpp:     mutex.h
smartptr.cpp:  smartptr.h mutex.h

clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul
