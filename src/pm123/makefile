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
OBJECTS  = $(OBJECTS) properties$(CO) skin$(CO) pipe$(CO)
OBJECTS  = $(OBJECTS) playable$(CO) playlistmenu$(CO)
OBJECTS  = $(OBJECTS) docking$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
STDLIBS  = $(LIBS_UNI)

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
123_util.cpp:	..\utils\utilfct.h ..\utils\snprintf.h ..\utils\cpp/container.h pm123.h playable.h
pm123.h:	..\include\config.h ..\include\decoder_plug.h properties.h playable.h
playable.h:	..\utils\cpp/event.h ..\utils\utilfct.h ..\xio123\xio.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\cpp/queue.h ..\utils\cpp/cpputil.h ..\utils\cpp/container.h ..\utils\cpp/url123.h ..\utils\strutils.h ..\include\decoder_plug.h
controller.cpp:	plugman.h controller.h properties.h pm123.h ..\utils\cpp/mutex.h ..\utils\cpp/xstring.h
plugman.h:	..\include\plugin.h ..\include\format.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\include\visual_plug.h ..\utils\utilfct.h ..\utils\cpp/event.h
controller.h:	playable.h plugman.h ..\include\decoder_plug.h ..\utils\cpp/queue.h ..\utils\cpp/xstring.h ..\utils\cpp/event.h ..\utils\cpp/cpputil.h ..\utils\debuglog.h
docking.cpp:	pm123.h properties.h docking.h ..\utils\utilfct.h
iniman.cpp:	..\utils\utilfct.h pm123.h iniman.h plugman.h properties.h
playable.cpp:	plugman.h playable.h pm123.h copyright.h ..\xio123\xio.h ..\utils\debuglog.h
playlistbase.cpp:	..\utils\utilfct.h playlistbase.h playlistmanager.h playlistview.h playable.h pm123.h 123_util.h pm123.rc.h docking.h iniman.h plugman.h ..\utils\snprintf.h ..\utils\cpp/smartptr.h ..\utils\cpp/url123.h ..\utils\debuglog.h
playlistbase.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h ..\utils\cpp/url123.h playable.h controller.h ..\include\decoder_plug.h ..\utils\debuglog.h
playlistmanager.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playlistbase.h
playlistview.h:	..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h playable.h playlistbase.h ..\include\decoder_plug.h
123_util.h:	..\include\decoder_plug.h
playlistmanager.cpp:	pm123.h pm123.rc.h ..\utils\utilfct.h playlistmanager.h docking.h iniman.h playable.h plugman.h ..\utils\cpp/smartptr.h ..\utils\cpp/showaccel.h ..\utils\debuglog.h
playlistmenu.cpp:	pm123.h pm123.rc.h playlistmenu.h ..\utils\debuglog.h
playlistmenu.h:	playable.h ..\utils\vdelegate.h ..\utils\cpp/smartptr.h ..\utils\cpp/container.h ..\utils\cpp/xstring.h ..\utils\cpp/cpputil.h
playlistview.cpp:	pm123.h pm123.rc.h ..\utils\utilfct.h playlistview.h docking.h iniman.h playable.h plugman.h ..\utils\cpp/showaccel.h ..\utils\debuglog.h
plugman.cpp:	..\utils\utilfct.h ..\include\format.h ..\utils\fileutil.h ..\utils\cpp/mutex.h plugman_base.h pm123.h playable.h pm123.rc.h ..\utils\debuglog.h
plugman_base.h:	plugman.h ..\utils\vdelegate.h
plugman_base.cpp:	..\utils\utilfct.h plugman_base.h pm123.h playable.h controller.h ..\utils\vdelegate.h ..\utils\debuglog.h
pm123.cpp:	..\utils\utilfct.h ..\xio123\xio.h pm123.h 123_util.h plugman.h button95.h playlistmanager.h playlistview.h copyright.h docking.h iniman.h controller.h playlistmenu.h skin.h pipe.h ..\include\visual_plug.h ..\utils\cpp/xstring.h ..\utils\cpp/url123.h ..\utils\cpp/showaccel.h pm123.rc.h ..\utils\debuglog.h
skin.h:	playable.h
properties.cpp:	..\utils\utilfct.h ..\xio123\xio.h ..\utils\snprintf.h properties.h pm123.h pm123.rc.h iniman.h plugman.h controller.h copyright.h 123_util.h pipe.h
skin.cpp:	..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h plugman.h skin.h pm123.h pm123.rc.h button95.h ..\utils\debuglog.h
button95.c:	button95.h ..\utils\utilfct.h
test.c:	..\utils\vdelegate.h ..\utils\vdelegate.c
pipe.cpp:	pipe.h pm123.h pm123.rc.h properties.h controller.h 123_util.h ..\utils\cpp/xstring.h ..\utils\debuglog.h
pipe.h:	..\include\config.h
