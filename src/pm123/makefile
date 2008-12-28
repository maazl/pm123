#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET   = pm123.exe

!include $(TOPDIR)\config\makerules

OBJECTS  = playable$(CO) playablecollection$(CO) songiterator$(CO)
OBJECTS  = $(OBJECTS) windowbase$(CO) playlistbase$(CO) playlistview$(CO) playlistmanager$(CO)
OBJECTS  = $(OBJECTS) plugman_base$(CO) plugman$(CO) glue$(CO)
OBJECTS  = $(OBJECTS) properties$(CO) iniman$(CO) docking$(CO) button95$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) controller$(CO) 123_util$(CO) pipe$(CO) pm123$(CO)
OBJECTS  = $(OBJECTS) playlistmenu$(CO) dialog$(CO) filedlg$(CO) infodialog$(CO) inspector$(CO)
DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\xio123\xio123$(LBI) $(TOPDIR)\gbm123\libgbm$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
STDLIBS  = $(LIBS_UNI)

pm123.exe: $(OBJECTS) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_PM) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

#test.exe: test.obj $(DEF) $(LIBS)
#	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)test.obj $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

#plugman$(CO):	plugman.cpp
#plugman_base$(CO):	plugman_base.cpp


clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul

# DO NOT DELETE

# include depenencies
pm123$(RES):    pm123.rc pm123.rc.h
123_util.cpp:	..\utils\utilfct.h ..\utils\snprintf.h 123_util.h pm123.h playable.h copyright.h skin.h controller.h ..\include\visual_plug.h
pm123.h:	..\include\config.h playablecollection.h controller.h
playable.h:	..\utils\cpp/event.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\cpp/queue.h ..\utils\cpp/cpputil.h ..\utils\cpp/container/sorted_vector.h ..\utils\cpp/container/inst_index.h ..\utils\cpp/url123.h ..\include\decoder_plug.h
controller.cpp:	plugman.h controller.h properties.h pm123.h ..\utils\inimacro.h ..\utils\cpp/mutex.h ..\utils\cpp/xstring.h
plugman.h:	playable.h ..\include\plugin.h ..\include\format.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\include\visual_plug.h ..\utils\utilfct.h ..\utils\cpp/event.h ..\utils\cpp/xstring.h
controller.h:	playable.h glue.h songiterator.h ..\include\decoder_plug.h ..\utils\cpp/queue.h ..\utils\cpp/xstring.h ..\utils\cpp/event.h ..\utils\cpp/mutex.h ..\utils\cpp/cpputil.h ..\utils\debuglog.h
docking.cpp:	dialog.h properties.h docking.h ..\utils\utilfct.h
iniman.cpp:	..\utils\utilfct.h pm123.h dialog.h iniman.h plugman.h properties.h
playable.cpp:	playable.h pm123.h properties.h ..\utils\strutils.h ..\utils\utilfct.h ..\utils\debuglog.h
playlistbase.cpp:	..\utils\utilfct.h playlistbase.h playlistmanager.h playlistview.h infodialog.h playable.h pm123.h properties.h 123_util.h pm123.rc.h docking.h iniman.h filedlg.h ..\utils\snprintf.h ..\utils\cpp/smartptr.h ..\utils\cpp/url123.h ..\utils\debuglog.h
playlistbase.h:	windowbase.h playable.h plugman.h controller.h ..\include\decoder_plug.h ..\utils\cpp/queue.h ..\utils\cpp/event.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/container/vector.h ..\utils\cpp/container/sorted_vector.h ..\utils\cpp/xstring.h ..\utils\cpp/url123.h ..\utils\debuglog.h
playlistmanager.h:	..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h playlistbase.h
playlistview.h:	..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h playable.h playlistbase.h ..\include\decoder_plug.h
123_util.h:	..\include\decoder_plug.h ..\utils\cpp/xstring.h ..\utils\cpp/url123.h
playlistmanager.cpp:	pm123.h dialog.h properties.h pm123.rc.h ..\utils\utilfct.h playlistmanager.h docking.h iniman.h playable.h ..\utils\cpp/smartptr.h ..\utils\cpp/showaccel.h ..\utils\debuglog.h
playlistmenu.cpp:	pm123.h pm123.rc.h playlistmenu.h ..\utils\utilfct.h ..\utils\debuglog.h
playlistmenu.h:	playable.h ..\utils\vdelegate.h ..\utils\cpp/smartptr.h ..\utils\cpp/container/sorted_vector.h ..\utils\cpp/xstring.h ..\utils\cpp/cpputil.h
playlistview.cpp:	pm123.h dialog.h properties.h pm123.rc.h ..\utils\utilfct.h playlistview.h docking.h iniman.h playable.h ..\utils\cpp/showaccel.h ..\utils\debuglog.h
plugman.cpp:	plugman.h plugman_base.h 123_util.h pm123.h dialog.h properties.h pm123.rc.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp/mutex.h ..\utils\cpp/url123.h ..\utils\cpp/stringmap.h ..\utils\debuglog.h
plugman_base.h:	plugman.h ..\utils\vdelegate.h ..\utils\cpp/smartptr.h
plugman_base.cpp:	..\utils\utilfct.h plugman_base.h pm123.h dialog.h playable.h controller.h ..\utils\vdelegate.h ..\utils\cpp/url123.h ..\utils\debuglog.h
pm123.cpp:	..\utils\utilfct.h ..\xio123\xio.h pm123.h dialog.h 123_util.h plugman.h button95.h playlistmanager.h playlistview.h infodialog.h inspector.h copyright.h docking.h iniman.h properties.h controller.h playlistmenu.h skin.h pipe.h ..\include\visual_plug.h ..\utils\cpp/xstring.h ..\utils\cpp/url123.h ..\utils\cpp/showaccel.h pm123.rc.h ..\utils\debuglog.h
skin.h:	playable.h
properties.cpp:	..\utils\utilfct.h ..\xio123\xio.h ..\utils\snprintf.h properties.h pm123.h dialog.h pm123.rc.h iniman.h plugman.h plugman_base.h controller.h copyright.h 123_util.h pipe.h filedlg.h ..\utils\cpp/url123.h ..\utils\cpp/stringmap.h
skin.cpp:	..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h properties.h skin.h pm123.h dialog.h pm123.rc.h button95.h plugman.h ..\utils\strutils.h ..\utils\debuglog.h
button95.c:	button95.h ..\utils\utilfct.h ..\utils\debuglog.h
pipe.cpp:	pipe.h dialog.h pm123.h pm123.rc.h properties.h controller.h 123_util.h ..\utils\utilfct.h ..\utils\cpp/xstring.h ..\utils\debuglog.h
pipe.h:	..\include\config.h ..\utils\cpp/xstring.h playablecollection.h
songiterator.h:	..\include\format.h playable.h playablecollection.h ..\utils\cpp/container/vector.h ..\utils\cpp/xstring.h
docking.mm.cpp:	pm123.h properties.h docking.h ..\utils\utilfct.h
songiterator.cpp:	songiterator.h properties.h ..\utils\cpp/mutex.h ..\utils\cpp/xstring.h ..\utils\strutils.h
playablecollection.h:	playable.h ..\utils\cpp/event.h ..\xio123\xio.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\cpp/cpputil.h ..\utils\cpp/container/sorted_vector.h ..\utils\cpp/container/list.h ..\utils\cpp/url123.h ..\include\decoder_plug.h ..\include\format.h
playablecollection.cpp:	playablecollection.h songiterator.h properties.h 123_util.h copyright.h ..\xio123\xio.h ..\utils\strutils.h ..\utils\snprintf.h ..\utils\debuglog.h
test.cpp:	..\utils\cpp/smartptr.h
dialog.cpp:	dialog.h ..\utils\utilfct.h ..\utils\cpp/xstring.h ..\utils\cpp/stringmap.h properties.h controller.h copyright.h skin.h iniman.h pm123.h 123_util.h filedlg.h pm123.rc.h ..\utils\debuglog.h
dialog.h:	..\include\config.h playablecollection.h
windowbase.h:	..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h ..\utils\debuglog.h
windowbase.cpp:	..\utils\utilfct.h windowbase.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\debuglog.h
infodialog.h:	windowbase.h playable.h ..\utils\cpp/container/inst_index.h ..\utils\cpp/smartptr.h ..\utils\cpp/mutex.h ..\utils\cpp/event.h
infodialog.cpp:	infodialog.h pm123.h pm123.rc.h glue.h ..\include\decoder_plug.h ..\include\plugin.h ..\utils\utilfct.h
glue.h:	..\include\output_plug.h ..\include\decoder_plug.h playable.h ..\utils\cpp/stringmap.h
iniman.h:	..\utils\cpp/xstring.h
glue.cpp:	glue.h plugman_base.h 123_util.h properties.h pm123.h ..\utils\fileutil.h ..\utils\eautils.h ..\utils\debuglog.h
properties.h:	..\include\config.h
filedlg.cpp:	filedlg.h pm123.rc.h iniman.h properties.h dialog.h ..\utils\utilfct.h ..\utils\debuglog.h
inspector.cpp:	inspector.h pm123.rc.h pm123.h songiterator.h properties.h ..\utils\utilfct.h ..\utils\snprintf.h ..\utils\cpp/Mutex.h ..\utils\debuglog.h
inspector.h:	windowbase.h controller.h ..\utils\cpp/smartptr.h ..\utils\cpp/queue.h
