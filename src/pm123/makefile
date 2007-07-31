#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) messages$(CO) 123_util$(CO)
OBJECTS  = $(OBJECTS) pfreq$(CO) pfreq_base$(CO) plugman$(CO) plugman_base$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) playable$(CO) playenumerator$(CO) url$(CO) repository$(CO)
OBJECTS  = $(OBJECTS) docking$(CO) vdelegate$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
#STDLIBS  = $(LIBS_UNI)

all: pm123.exe $(MDUMMY)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

plugman$(CO):	plugman.cpp
plugman_base$(CO):	plugman_base.cpp

123_util.cpp:   pm123.h plugman.h playable.h
plugman.cpp:    pm123.h plugman.h plugman_base.h
plugman_base.cpp: plugman.h plugman_base.h vdelegate.h
pm123.cpp:      pm123.h plugman.h bookmark.h button95.h pfreq.h docking.h copyright.h messages.h playenumerator.h
docking.c:      docking.h
iniman.c:       pm123.h plugman.h iniman.h
playlist.cpp:   pm123.h playlist.h
properties.c:   pm123.h plugman.h copyright.h properties.h
bookmark.cpp:   pm123.h bookmark.h
skin.cpp:       pm123.h plugman.h
vdelegate.c:    vdelegate.h
pipe.c:         playlist.h pm123.h properties.h
messages.cpp:   pm123.h messages.h
messages.h:     playable.h
pm123.rc:       pm123.h bookmark.h
pm123.h:        playlist.h skin.h properties.h playable.h
playable.cpp:   playable.h playenumerator.h playable.private.h repository.h
playable.h:     url.h repository.h
playenumerator.cpp:	playenumerator.h
playenumerator.h:	playable.h
pfreq.cpp:      pfreq.h pfreq_base.h url.h
pfreq_base.cpp: pm123.h pfreq_base.h iniman.h playable.h url.h
pfreq_base.h:   playable.h repository.h
url.cpp:        url.h
repository.cpp: repository.h

clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul
# DO NOT DELETE
