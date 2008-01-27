#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
EXE_PM   = yes
TARGET   = pm123.exe

!include $(TOPDIR)\config\makerules

OBJECTS  = button95$(CO) pm123$(CO) 123_util$(CO) controller$(CO)
OBJECTS  = $(OBJECTS) plugman$(CO) plugman_base$(CO) iniman$(CO)
OBJECTS  = $(OBJECTS) playlistbase$(CO) playlistview$(CO) playlistmanager$(CO)
OBJECTS  = $(OBJECTS) properties$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) playable$(CO) url$(CO) playlistmenu$(CO)
OBJECTS  = $(OBJECTS) docking$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
#STDLIBS  = $(LIBS_UNI)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

#plugman$(CO):	plugman.cpp
#plugman_base$(CO):	plugman_base.cpp


clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul

# DO NOT DELETE

# include depenencies
pm123$(RES):    pm123.rc pm123.rc.h
123_util.cpp:	..\utils\utilfct.h pm123.h playable.h
pm123.h:	plugman.h properties.h copyright.h ..\utils\cpp/xstring.h playable.h
plugman.h:	..\include\plugin.h ..\include\format.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\utils\utilfct.h
playable.h:	..\utils\cpp/event.h ..\utils\utilfct.h ..\xio123\xio.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\cpp/queue.h ..\utils\cpp/cpputil.h ..\utils\cpp/container.h ..\utils\strutils.h url.h ..\include\decoder_plug.h
url.h:	..\utils\cpp/xstring.h
controller.cpp:	plugman.h controller.h properties.h ..\utils\cpp/xstring.h
controller.h:	playable.h ..\include\decoder_plug.h ..\utils\cpp/queue.h ..\utils\cpp/xstring.h ..\utils\cpp/event.h ..\utils\cpp/cpputil.h ..\utils\debuglog.h
playable.cpp:	plugman.h playable.h pm123.h ..\xio123\xio.h ..\utils\debuglog.h
playlistbase.cpp:	..\utils\utilfct.h playlistbase.h playlistmanager.h playlistview.h playable.h pm123.h pm123.rc.h docking.h iniman.h ..\utils\snprintf.h ..\utils\cpp/smartptr.h url.h ..\utils\debuglog.h
playlistbase.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playable.h controller.h ..\include\decoder_plug.h ..\utils\debuglog.h
playlistmanager.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playlistbase.h
playlistview.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playable.h playlistbase.h ..\include\decoder_plug.h
playlistmanager.cpp:	pm123.h pm123.rc.h ..\utils\utilfct.h playlistmanager.h docking.h iniman.h playable.h ..\utils\cpp/smartptr.h url.h ..\utils\debuglog.h
playlistmenu.cpp:	pm123.h pm123.rc.h playlistmenu.h ..\utils\debuglog.h
playlistmenu.h:	playable.h ..\utils\vdelegate.h ..\utils\cpp/smartptr.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h ..\utils\cpp/cpputil.h
playlistview.cpp:	pm123.h pm123.rc.h ..\utils\utilfct.h playlistview.h docking.h iniman.h playable.h ..\utils\debuglog.h
plugman.cpp:	..\utils\utilfct.h ..\include\format.h ..\utils\fileutil.h plugman_base.h pm123.h pm123.rc.h ..\utils\debuglog.h
plugman_base.h:	plugman.h ..\utils\vdelegate.h
plugman_base.cpp:	..\utils\utilfct.h plugman_base.h pm123.h ..\utils\vdelegate.h ..\utils\debuglog.h
pm123.cpp:	..\utils\utilfct.h ..\xio123\xio.h pm123.h plugman.h button95.h playlistmanager.h playlistview.h copyright.h docking.h iniman.h controller.h playlistmenu.h skin.h ..\utils\cpp/xstring.h url.h pm123.rc.h ..\utils\debuglog.h
skin.h:	playable.h
skin.cpp:	..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h plugman.h skin.h pm123.h pm123.rc.h button95.h ..\utils\debuglog.h
url.cpp:	url.h ..\utils\strutils.h
button95.c:	pm123.h button95.h
docking.c:	pm123.h docking.h ..\utils\utilfct.h
iniman.cpp:	..\utils\utilfct.h pm123.h iniman.h plugman.h
properties.cpp:	..\xio123\xio.h properties.h pm123.h pm123.rc.h iniman.h
test.c:	..\utils\vdelegate.h ..\utils\vdelegate.c
