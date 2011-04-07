#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET   = pm123.exe
CFLAGS_USR = -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJECTS  = infobundle$(CO) infostate$(CO) aplayable$(CO) waitinfo$(CO) playableset$(CO)
OBJECTS  = $(OBJECTS) location$(CO) collectioninfocache$(CO) playable$(CO) playableref$(CO)
OBJECTS  = $(OBJECTS) windowbase$(CO) playlistbase$(CO) playlistview$(CO) playlistmanager$(CO)
OBJECTS  = $(OBJECTS) plugman_base$(CO) plugman$(CO) glue$(CO)
OBJECTS  = $(OBJECTS) controller$(CO) dstring$(CO) 123_util$(CO) pipe$(CO)
OBJECTS  = $(OBJECTS) properties$(CO) docking$(CO) button95$(CO) skin$(CO)
OBJECTS  = $(OBJECTS) playlistmenu$(CO) filedlg$(CO) dialog$(CO) infodialog$(CO) inspector$(CO)
OBJECTS  = $(OBJECTS) loadhelper$(CO) gui$(CO) pm123$(CO)
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


clean: $(MDUMMY)
	-@del pm123.exe $(OBJECTS) $(RESOURCE) 2> nul

# DO NOT DELETE

# include depenencies
pm123$(RES):    pm123.rc pm123.rc.h
123_util.cpp:	..\utils\utilfct.h ..\utils\snprintf.h 123_util.h gui.h copyright.h skin.h controller.h ..\include\visual_plug.h ..\utils\interlocked.h
..\utils\utilfct.h:	..\include\config.h ..\utils\errorstr.h ..\utils\minmax.h ..\utils\strutils.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
123_util.h:	..\include\decoder_plug.h ..\utils\cpp\xstring.h ..\utils\cpp\url123.h
..\include\decoder_plug.h:	..\include\format.h ..\include\output_plug.h
..\include\plugin.h:	..\include\config.h ..\utils\cpp\xstring.h
..\utils\cpp/xstring.h:	..\utils\cpp/smartptr.h
..\utils\cpp/smartptr.h:	..\include\config.h ..\utils\cpp/cpputil.h ..\utils\cpp/mutex.h ..\utils\debuglog.h
..\utils\cpp/mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
..\utils\interlocked.h:	..\include\config.h
..\utils\debuglog.h:	..\utils\errorstr.h
..\include\output_plug.h:	..\include\format.h
..\utils\cpp/url123.h:	..\utils\cpp/xstring.h ..\utils\cpp/container/stringmap.h
..\utils\cpp/container/stringmap.h:	..\utils\cpp/xstring.h ..\utils\cpp/container/sorted_vector.h
..\utils\cpp/container/sorted_vector.h:	..\utils\cpp/smartptr.h ..\utils\cpp/algorithm.h ..\utils\cpp/container/vector.h
..\utils\cpp/algorithm.h:	..\utils\cpp/container/vector.h
..\utils\cpp/container/vector.h:	..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/xstring.h
..\utils\cpp/event.h:	..\include\config.h ..\utils\cpp/mutex.h ..\utils\debuglog.h
controller.h:	aplayable.h location.h glue.h ..\include\decoder_plug.h ..\utils\cpp\queue.h ..\utils\cpp\xstring.h ..\utils\cpp\event.h ..\utils\cpp\mutex.h ..\utils\cpp\cpputil.h ..\utils\debuglog.h
aplayable.h:	infostate.h ..\utils\cpp\event.h ..\utils\cpp\atomic.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\queue.h ..\utils\cpp\cpputil.h ..\utils\cpp\url123.h
..\utils\cpp/atomic.h:	..\include\config.h ..\utils\interlocked.h
..\utils\cpp/queue.h:	..\include\config.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\debuglog.h
location.h:	..\include\format.h aplayable.h playableset.h ..\utils\cpp\container\vector.h
glue.h:		..\include\output_plug.h ..\include\decoder_plug.h playable.h
playableref.h:	aplayable.h playableset.h collectioninfocache.h ..\utils\cpp\event.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h
playableset.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp/container/inst_index.h:	..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/container/sorted_vector.h
..\utils\cpp/container/list.h:	..\utils\cpp/smartptr.h
properties.h:	..\include\config.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h
plugman.h:	pipe.h ..\include\plugin.h ..\include\decoder_plug.h ..\utils\vdelegate.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\event.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h
pipe.h:		..\include\config.h ..\utils\cpp\xstring.h
..\utils\vdelegate.h:	..\include\format.h
loadhelper.h:	..\utils\cpp\smartptr.h controller.h
playlistmenu.h:	playable.h ..\utils\vdelegate.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h
..\include\visual_plug.h:	..\include\decoder_plug.h
aplayable.cpp:	properties.h aplayable.h playableset.h waitinfo.h pm123.h ..\utils\utilfct.h ..\utils\debuglog.h
pm123.h:	..\include\config.h ..\utils\cpp\xstring.h
controller.cpp:	plugman.h waitinfo.h controller.h properties.h gui.h pm123.h ..\utils\inimacro.h ..\utils\minmax.h ..\utils\utilfct.h ..\utils\cpp\mutex.h ..\utils\cpp\xstring.h
..\utils\inimacro.h:	..\include\config.h
dialog.cpp:	dialog.h properties.h controller.h copyright.h skin.h gui.h 123_util.h filedlg.h ..\utils\utilfct.h ..\utils\fileutil.h ..\xio123\xio.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h pm123.rc.h ..\utils\debuglog.h
dialog.h:	..\include\config.h ..\include\decoder_plug.h ..\utils\cpp\url123.h
filedlg.h:	..\include\decoder_plug.h ..\utils\cpp\container\vector.h
..\utils\fileutil.h:	..\include\config.h
..\xio123\xio.h:	..\include\config.h
docking.cpp:	dialog.h properties.h docking.h ..\utils\utilfct.h
dstring.cpp:	dstring.h ..\utils\interlocked.h ..\utils\cpp\xstring.h
dstring.h:	..\include\config.h ..\include\format.h ..\utils\cpp\xstring.h
filedlg.cpp:	filedlg.h pm123.rc.h properties.h dialog.h glue.h ..\utils\utilfct.h ..\utils\snprintf.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
glue.cpp:	glue.h plugman_base.h 123_util.h properties.h pm123.h dstring.h ..\utils\fileutil.h ..\utils\eautils.h ..\xio123\xio.h ..\utils\cpp\container\sorted_vector.h ..\utils\debuglog.h
plugman_base.h:	plugman.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\include\visual_plug.h ..\utils\vdelegate.h ..\utils\cpp\smartptr.h
..\include\filter_plug.h:	..\include\format.h ..\include\output_plug.h
infodialog.h:	windowbase.h
windowbase.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\debuglog.h
playlistview.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h playable.h playlistbase.h ..\include\decoder_plug.h
playlistbase.h:	windowbase.h playable.h infodialog.h plugman.h controller.h ..\include\decoder_plug.h ..\utils\cpp\queue.h ..\utils\cpp\event.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\url123.h ..\utils\debuglog.h
playlistmanager.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h playlistbase.h
inspector.h:	windowbase.h ..\utils\cpp\smartptr.h ..\utils\cpp\queue.h ..\utils\cpp\container\vector.h
..\utils\cpp/showaccel.h:	..\utils\cpp/container/sorted_vector.h
infobundle.cpp:	123_util.h infobundle.h playableset.h playable.h dstring.h ..\utils\debuglog.h
infodialog.cpp:	infodialog.h waitinfo.h pm123.h gui.h pm123.rc.h glue.h properties.h ..\include\decoder_plug.h ..\include\plugin.h ..\utils\utilfct.h
inspector.cpp:	inspector.h pm123.rc.h pm123.h properties.h controller.h ..\utils\utilfct.h ..\utils\snprintf.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\cpp/Mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
loadhelper.cpp:	loadhelper.h properties.h gui.h
pipe.cpp:	pipe.h dialog.h gui.h loadhelper.h properties.h controller.h 123_util.h plugman.h ..\utils\fileutil.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
playablecollection.cpp:	playablecollection.h songiterator.h properties.h 123_util.h copyright.h ..\xio123\xio.h ..\utils\strutils.h ..\utils\snprintf.h ..\utils\charset.h ..\utils\debuglog.h
playableref.cpp:	properties.h playableref.h playable.h location.h waitinfo.h ..\utils\utilfct.h ..\utils\debuglog.h
playableset.cpp:	playableset.h playable.h ..\utils\debuglog.h
playlistbase.cpp:	..\utils\utilfct.h playlistbase.h playlistmanager.h playlistview.h infodialog.h playable.h waitinfo.h pm123.h gui.h properties.h 123_util.h pm123.rc.h docking.h filedlg.h ..\utils\cpp\smartptr.h ..\utils\cpp\url123.h ..\utils\snprintf.h ..\utils\debuglog.h
playlistmanager.cpp:	playlistmanager.h gui.h dialog.h properties.h pm123.rc.h docking.h playable.h ..\utils\utilfct.h ..\utils\cpp\smartptr.h ..\utils\cpp\showaccel.h ..\utils\debuglog.h
playlistmenu.cpp:	pm123.h pm123.rc.h playlistmenu.h ..\utils\utilfct.h ..\utils\debuglog.h
playlistview.cpp:	playlistview.h gui.h dialog.h properties.h pm123.rc.h docking.h playable.h ..\utils\cpp\showaccel.h ..\utils\utilfct.h ..\utils\debuglog.h
plugman.cpp:	plugman.h plugman_base.h 123_util.h dstring.h pm123.h dialog.h properties.h pm123.rc.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp\mutex.h ..\utils\cpp\url123.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
plugman_base.cpp:	plugman_base.h pm123.h dstring.h dialog.h properties.h 123_util.h playable.h controller.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\charset.h ..\utils\vdelegate.h ..\utils\cpp\url123.h ..\utils\debuglog.h
pm123.cpp:	pm123.h 123_util.h plugman.h button95.h gui.h properties.h controller.h pipe.h ..\utils\fileutil.h ..\utils\utilfct.h ..\utils\debuglog.h
properties.cpp:	properties.h pm123.h gui.h dialog.h docking.h pm123.rc.h plugman.h plugman_base.h controller.h copyright.h 123_util.h pipe.h filedlg.h ..\utils\utilfct.h ..\utils\errorstr.h ..\utils\inimacro.h ..\xio123\xio.h ..\utils\snprintf.h ..\utils\cpp\url123.h ..\utils\cpp\container\stringmap.h
skin.cpp:	skin.h properties.h glue.h pm123.h dialog.h pm123.rc.h button95.h plugman.h ..\include\visual_plug.h ..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h ..\utils\minmax.h ..\utils\strutils.h ..\utils\fileutil.h ..\include\sys\io.h ..\utils\debuglog.h
windowbase.cpp:	..\utils\utilfct.h windowbase.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
button95.c:	button95.h ..\utils\utilfct.h ..\utils\debuglog.h
playable.h:	..\include\config.h aplayable.h playableref.h collectioninfocache.h ..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\cpputil.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\container\list.h ..\utils\cpp\url123.h
location.cpp:	location.h playable.h waitinfo.h
..\utils\cpp\xstring.h:	..\utils\cpp\smartptr.h
..\utils\cpp\smartptr.h:	..\include\config.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\cpp\mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
..\utils\cpp\url123.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h
..\utils\cpp\container\stringmap.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp\container\sorted_vector.h:	..\utils\cpp\smartptr.h ..\utils\cpp\algorithm.h ..\utils\cpp\container\vector.h
..\utils\cpp\algorithm.h:	..\utils\cpp\container\vector.h
..\utils\cpp\container\vector.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h
..\utils\cpp\event.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
infostate.h:	infobundle.h ..\utils\cpp\event.h ..\utils\cpp\atomic.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\queue.h ..\utils\cpp\cpputil.h ..\utils\cpp\url123.h
..\utils\cpp\atomic.h:	..\include\config.h ..\utils\interlocked.h
..\utils\cpp\queue.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\debuglog.h
..\utils\cpp\container\inst_index.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp\container\list.h:	..\utils\cpp\smartptr.h ..\utils\cpp\cpputil.h
..\utils\cpp\showaccel.h:	..\utils\cpp\container\sorted_vector.h
infostate.cpp:	infostate.h ..\utils\debuglog.h
collectioninfocache.h:	..\include\config.h aplayable.h infobundle.h playableset.h ..\utils\cpp\mutex.h ..\utils\cpp\container\sorted_vector.h
collectioninfocache.cpp:	collectioninfocache.h playable.h waitinfo.h ..\utils\utilfct.h ..\utils\cpp\algorithm.h ..\utils\debuglog.h
waitinfo.cpp:	waitinfo.h playableset.h ..\utils\debuglog.h
..\include\format.h:	..\include\plugin.h
infobundle.h:	..\include\format.h ..\include\decoder_plug.h ..\utils\cpp\cpputil.h ..\utils\cpp\xstring.h ..\utils\strutils.h
playable.cpp:	playable.h properties.h dstring.h glue.h waitinfo.h location.h ..\utils\strutils.h ..\utils\utilfct.h ..\utils\vdelegate.h ..\utils\cpp\algorithm.h ..\utils\debuglog.h
gui.h:		..\include\config.h ..\utils\cpp\smartptr.h ..\utils\cpp\event.h controller.h properties.h plugman.h loadhelper.h playlistmenu.h pm123.rc.h
waitinfo.h:	aplayable.h ..\utils\cpp\event.h ..\utils\cpp\mutex.h ..\utils\cpp\container\sorted_vector.h
gui.cpp:	gui.h dialog.h filedlg.h properties.h pm123.h pm123.rc.h 123_util.h glue.h skin.h docking.h infodialog.h playlistview.h playlistmanager.h inspector.h waitinfo.h button95.h copyright.h ..\utils\utilfct.h ..\utils\cpp\pmutils.h ..\utils\cpp\showaccel.h
