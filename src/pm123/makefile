#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
EXE_PM   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) messages$(CO) 123_util$(CO)
OBJECTS  = $(OBJECTS) playlistmanager$(CO) playlistmanager.i$(CO)
OBJECTS  = $(OBJECTS) plugman$(CO) plugman_base$(CO) bookmark$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlist$(CO) playlist_base$(CO) playlist.i$(CO) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) playable$(CO) playenumerator$(CO) url$(CO)
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


clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul

depend:
	..\config\makedep.cmd *.cpp *.c -r -x -I$(INCDIR)

# DO NOT DELETE

# include depenencies

123_util.cpp:	..\utils\utilfct.h pm123.h plugman.h playable.h
bookmark.cpp:	pm123.h pm123.rc.h bookmark.h ..\utils\utilfct.h plugman.h docking.h iniman.h messages.h playable.h
messages.cpp:	..\utils\debuglog.h pm123.h ..\utils\utilfct.h plugman.h messages.h
playable.cpp:	plugman.h playable.h playlist.h ..\xio123\xio.h ..\utils\debuglog.h playable.i.h
playenumerator.cpp:	playenumerator.h ..\utils\cpp/mutex.h ..\utils\debuglog.h
playlist.cpp:	..\xio123\xio.h plugman.h playlist.h pm123.h pm123.rc.h ..\utils\utilfct.h docking.h iniman.h ..\utils\debuglog.h
playlist.i.cpp:	pm123.h ..\utils\utilfct.h playlist.i.h docking.h iniman.h playable.h ..\utils\snprintf.h ..\utils\cpp/smartptr.h url.h ..\utils\debuglog.h
playlistmanager.cpp:	..\utils\utilfct.h pm123.h playlistmanager.h playlistmanager.i.h url.h ..\utils\debuglog.h
playlistmanager.i.cpp:	pm123.h pm123.rc.h ..\utils\utilfct.h playlistmanager.h playlistmanager.i.h docking.h iniman.h playable.h ..\utils\cpp/smartptr.h url.h ..\utils\debuglog.h
playlist_base.cpp:	..\utils\utilfct.h playlist_base.h playable.h pm123.rc.h docking.h ..\utils\snprintf.h ..\utils\cpp/smartptr.h url.h ..\utils\debuglog.h
plugman.cpp:	..\utils\utilfct.h ..\include\format.h ..\utils\fileutil.h plugman_base.h pm123.h pm123.rc.h ..\utils\debuglog.h
plugman_base.cpp:	..\utils\utilfct.h plugman_base.h pm123.h vdelegate.h ..\utils\debuglog.h
pm123.cpp:	..\utils\utilfct.h ..\xio123\xio.h pm123.h plugman.h bookmark.h button95.h playlistmanager.h copyright.h docking.h iniman.h messages.h playenumerator.h ..\utils\cpp/xstring.h url.h pm123.rc.h ..\utils\debuglog.h
skin.cpp:	..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h skin.h pm123.h pm123.rc.h button95.h ..\utils\debuglog.h
url.cpp:	url.h ..\utils\strutils.h
123_msg.c:	..\utils\utilfct.h pm123.h plugman.h ..\utils\debuglog.h
docking.c:	pm123.h docking.h ..\utils\utilfct.h
iniman.c:	..\utils\utilfct.h pm123.h iniman.h plugman.h
properties.c:	..\xio123\xio.h properties.h pm123.h pm123.rc.h iniman.h
button95.c:	pm123.h button95.h
test.c:		vdelegate.h vdelegate.c
vdelegate.c:	vdelegate.h
pm123.h:	skin.h plugman.h playlist.h properties.h copyright.h playable.h
plugman.h:	..\include\plugin.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\utils\utilfct.h
playlist.h:	..\include\format.h ..\include\decoder_plug.h
playable.h:	..\utils\cpp/event.h ..\utils\utilfct.h ..\xio123\xio.h ..\include\decoder_plug.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\cpp/queue.h ..\utils\cpp/cpputil.h ..\utils\cpp/container.h ..\utils\strutils.h url.h
url.h:		..\utils\cpp/xstring.h
messages.h:	playable.h
playable.i.h:	playenumerator.h
playenumerator.h:	playable.h ..\utils\cpp/smartptr.h ..\utils\cpp/event.h
playlist.i.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playable.h playlist_base.h ..\include\decoder_plug.h
playlist_base.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playable.h ..\include\decoder_plug.h ..\utils\debuglog.h
playlistmanager.i.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playlist_base.h
plugman_base.h:	plugman.h vdelegate.h
vdelegate.h:	..\include\format.h
