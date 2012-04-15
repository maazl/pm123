#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET   = pm123.exe
CFLAGS_USR = -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJECTS  = infobundle$(CO) infostate$(CO) aplayable$(CO) waitinfo$(CO) dependencyinfo$(CO)
OBJECTS  = $(OBJECTS) playableset$(CO) location$(CO) songiterator$(CO) collectioninfocache$(CO)
OBJECTS  = $(OBJECTS) playableinstance$(CO) playable$(CO) playableref$(CO)
OBJECTS  = $(OBJECTS) plugman$(CO) proxyhelper$(CO)
OBJECTS  = $(OBJECTS) decoder$(CO) filter$(CO) output$(CO) visual$(CO) glue$(CO)

OBJECTS2 = eventhandler$(CO) configuration$(CO)
OBJECTS2 = $(OBJECTS2) playlistbase$(CO) playlistview$(CO) playlistmanager$(CO)
OBJECTS2 = $(OBJECTS2) controller$(CO) 123_util$(CO) commandprocessor$(CO) acommandprocessor$(CO)
OBJECTS2 = $(OBJECTS2) pipe$(CO) properties$(CO) docking$(CO) button95$(CO)
OBJECTS2 = $(OBJECTS2) playlistmenu$(CO) filedlg$(CO) dialog$(CO) infodialog$(CO) inspector$(CO)
OBJECTS2 = $(OBJECTS2) loadhelper$(CO) skin$(CO) gui$(CO) pm123$(CO)

DEF      = pm123$(LDEF_SUFFX)
RESOURCE = pm123$(RES)
LIBS     = $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\gbm123\libgbm$(LBO)
STDLIBS  = $(LIBS_UNI)

# objects directly passed to the linker cause undefined symbols with gcc 3.3.5
pm123core$(LBO) : $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

pm123gui$(LBO) : $(OBJECTS2)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS2) $(LBENDL)

pm123.exe: pm123core$(LBO) pm123gui$(LBO) $(RESOURCE) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_PM) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)pm123core$(LBO) pm123gui$(LBO) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

#test.exe: test.obj $(DEF) $(LIBS)
#	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)test.obj $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)


clean: $(MDUMMY)
	-@del pm123.exe pm123core$(LBO) pm123gui$(LBO) $(OBJECTS) $(OBJECTS2) $(RESOURCE) 2> nul

todo.txt: $(MDUMMY)
	grep -i -A 5 TODO *.cpp *.h >todo.txt


# DO NOT DELETE

# include depenencies
pm123$(RES):    pm123.rc pm123.rc.h
123_util.cpp:	..\utils\utilfct.h ..\utils\snprintf.h 123_util.h gui.h copyright.h skin.h controller.h configuration.h ..\include\visual_plug.h ..\utils\interlocked.h
..\utils\utilfct.h:	..\include\config.h ..\utils\errorstr.h ..\utils\minmax.h ..\utils\strutils.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
123_util.h:	..\include\decoder_plug.h ..\utils\cpp\xstring.h ..\utils\cpp\url123.h
..\include\decoder_plug.h:	..\include\format.h ..\include\output_plug.h
..\include\plugin.h:	..\include\config.h
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
controller.h:	aplayable.h ..\utils\cpp\queue.h ..\utils\cpp\xstring.h ..\utils\cpp\event.h ..\utils\cpp\cpputil.h ..\utils\debuglog.h
aplayable.h:	infostate.h ..\utils\cpp\event.h ..\utils\cpp\atomic.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\queue.h ..\utils\cpp\cpputil.h ..\utils\cpp\url123.h
..\utils\cpp/atomic.h:	..\include\config.h ..\utils\interlocked.h
..\utils\cpp/queue.h:	..\include\config.h ..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\debuglog.h
location.h:	..\include\format.h aplayable.h playableinstance.h ..\utils\cpp\container\vector.h
glue.h:		..\include\output_plug.h ..\include\decoder_plug.h playable.h
playableref.h:	aplayable.h collectioninfocache.h ..\utils\cpp\event.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h
playableset.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp/container/inst_index.h:	..\utils\cpp/mutex.h ..\utils\cpp/smartptr.h ..\utils\cpp/container/sorted_vector.h
..\utils\cpp/container/list.h:	..\utils\cpp/smartptr.h
properties.h:	..\include\config.h
plugman.h:	..\include\plugin.h ..\utils\cpp\event.h ..\utils\cpp\xstring.h ..\utils\cpp\container\vector.h
..\utils\vdelegate.h:	..\include\config.h
loadhelper.h:	..\utils\cpp\smartptr.h controller.h
playlistmenu.h:	playable.h ..\utils\vdelegate.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h
..\include\visual_plug.h:	..\include\format.h ..\include\output_plug.h
aplayable.cpp:	aplayable.h playableset.h waitinfo.h dependencyinfo.h pm123.h configuration.h ..\utils\utilfct.h ..\utils\debuglog.h
pm123.h:	..\include\config.h ..\utils\cpp\xstring.h
controller.cpp:	controller.h dependencyinfo.h configuration.h decoder.h gui.h glue.h pm123.h ..\utils\inimacro.h ..\utils\minmax.h ..\utils\utilfct.h ..\utils\cpp\mutex.h ..\utils\cpp\xstring.h
..\utils\inimacro.h:	..\include\config.h
dialog.cpp:	dialog.h configuration.h controller.h copyright.h skin.h gui.h 123_util.h filedlg.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\xio123\xio.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h pm123.rc.h ..\utils\debuglog.h
dialog.h:	..\include\plugin.h ..\include\config.h ..\include\decoder_plug.h ..\utils\cpp\url123.h
filedlg.h:	..\include\decoder_plug.h ..\utils\cpp\container\vector.h
..\utils\fileutil.h:	..\include\config.h
..\xio123\xio.h:	..\include\config.h
docking.cpp:	docking.h configuration.h eventhandler.h ..\utils\utilfct.h ..\utils\cpp\mutex.h
filedlg.cpp:	filedlg.h pm123.rc.h configuration.h dialog.h plugman.h decoder.h ..\utils\utilfct.h ..\utils\snprintf.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
glue.cpp:	glue.h decoder.h filter.h output.h configuration.h eventhandler.h ..\utils\debuglog.h
plugman_base.h:	plugman.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\include\visual_plug.h ..\utils\vdelegate.h ..\utils\cpp\smartptr.h
..\include\filter_plug.h:	..\include\format.h ..\include\output_plug.h
infodialog.h:	playable.h ..\utils\cpp\windowbase.h ..\utils\cpp\container\sorted_vector.h
windowbase.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\container\vector.h ..\utils\debuglog.h
playlistview.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\container\inst_index.h playable.h playlistbase.h ..\include\decoder_plug.h
playlistbase.h:	playable.h infodialog.h controller.h ..\include\decoder_plug.h ..\utils\cpp\queue.h ..\utils\cpp\event.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\xstring.h ..\utils\cpp\url123.h ..\utils\cpp\windowbase.h ..\utils\debuglog.h
playlistmanager.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\container\inst_index.h playlistbase.h
inspector.h:	..\utils\cpp\windowbase.h ..\utils\cpp\smartptr.h ..\utils\cpp\queue.h ..\utils\cpp\container\vector.h
..\utils\cpp/showaccel.h:	..\utils\cpp/container/sorted_vector.h
infobundle.cpp:	123_util.h infobundle.h playableset.h playable.h ..\utils\debuglog.h
infodialog.cpp:	infodialog.h dependencyinfo.h pm123.h gui.h pm123.rc.h configuration.h eventhandler.h ..\include\decoder_plug.h ..\include\plugin.h ..\utils\utilfct.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\pmutils.h ..\utils\cpp\dlgcontrols.h ..\utils\cpp\cppvdelegate.h
inspector.cpp:	inspector.h pm123.rc.h pm123.h configuration.h controller.h location.h playable.h dependencyinfo.h ..\utils\utilfct.h ..\utils\snprintf.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\cpp/Mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
loadhelper.cpp:	loadhelper.h configuration.h gui.h
pipe.cpp:	pipe.h acommandprocessor.h configuration.h eventhandler.h
playablecollection.cpp:	playablecollection.h songiterator.h properties.h 123_util.h copyright.h ..\xio123\xio.h ..\utils\strutils.h ..\utils\snprintf.h ..\utils\charset.h ..\utils\debuglog.h
playableref.cpp:	properties.h playableref.h playable.h location.h dependencyinfo.h ..\utils\debuglog.h
playableset.cpp:	playableset.h playable.h ..\utils\debuglog.h
playlistbase.cpp:	..\utils\utilfct.h playlistbase.h playlistmanager.h playlistview.h infodialog.h inspector.h playable.h dependencyinfo.h loadhelper.h pm123.h gui.h configuration.h 123_util.h pm123.rc.h docking.h filedlg.h ..\utils\cpp\smartptr.h ..\utils\cpp\url123.h ..\utils\snprintf.h ..\utils\debuglog.h
playlistmanager.cpp:	playlistmanager.h gui.h dialog.h configuration.h pm123.rc.h docking.h playable.h decoder.h ..\utils\utilfct.h ..\utils\cpp\smartptr.h ..\utils\cpp\showaccel.h ..\utils\debuglog.h
playlistmenu.cpp:	pm123.h pm123.rc.h playlistmenu.h ..\utils\cpp\cppvdelegate.h ..\utils\utilfct.h ..\utils\debuglog.h
playlistview.cpp:	playlistview.h gui.h dialog.h properties.h pm123.rc.h docking.h playable.h decoder.h ..\utils\cpp\showaccel.h ..\utils\utilfct.h ..\utils\debuglog.h
plugman.cpp:	plugman.h decoder.h filter.h output.h visual.h playable.h acommandprocessor.h configuration.h eventhandler.h proxyhelper.h pm123.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\url123.h ..\utils\cpp\cppvdelegate.h ..\utils\fileutil.h ..\utils\debuglog.h
plugman_base.cpp:	plugman_base.h pm123.h xstring_api.h dialog.h configuration.h 123_util.h playable.h controller.h glue.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\charset.h ..\utils\vdelegate.h ..\utils\cpp\url123.h ..\utils\debuglog.h
pm123.cpp:	pm123.h eventhandler.h 123_util.h button95.h plugman.h gui.h dialog.h configuration.h controller.h loadhelper.h pipe.h acommandprocessor.h ..\xio123\xio.h ..\utils\utilfct.h ..\utils\debuglog.h
properties.cpp:	properties.h configuration.h pm123.rc.h eventhandler.h dialog.h filedlg.h gui.h 123_util.h controller.h copyright.h plugman.h decoder.h visual.h pm123.h ..\utils\utilfct.h ..\utils\cpp\pmutils.h ..\utils\cpp\windowbase.h ..\utils\cpp\dlgcontrols.h
skin.cpp:	skin.h configuration.h glue.h pm123.h eventhandler.h pm123.rc.h button95.h plugman.h visual.h dialog.h ..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h ..\include\visual_plug.h ..\utils\minmax.h ..\utils\strutils.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\utils\cpp\cpputil.h ..\utils\debuglog.h
windowbase.cpp:	..\utils\utilfct.h windowbase.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
button95.cpp:	button95.h ..\utils\utilfct.h ..\utils\debuglog.h
playable.h:	..\include\config.h aplayable.h playableref.h playableinstance.h collectioninfocache.h ..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\cpputil.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\container\sorted_vector.h ..\utils\cpp\container\list.h ..\utils\cpp\url123.h
location.cpp:	location.h playable.h dependencyinfo.h
..\utils\cpp\xstring.h:	..\utils\cpp\smartptr.h
..\utils\cpp\smartptr.h:	..\include\config.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\cpp\mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
..\utils\cpp\url123.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h
..\utils\cpp\container\stringmap.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp\container\sorted_vector.h:	..\utils\cpp\smartptr.h ..\utils\cpp\algorithm.h ..\utils\cpp\container\vector.h
..\utils\cpp\algorithm.h:	..\utils\cpp\container\vector.h
..\utils\cpp\container\vector.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h
..\utils\cpp\event.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
infostate.h:	infobundle.h ..\utils\cpp\event.h ..\utils\cpp\atomic.h ..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\queue.h ..\utils\cpp\cpputil.h ..\utils\cpp\url123.h
..\utils\cpp\atomic.h:	..\include\config.h ..\utils\interlocked.h
..\utils\cpp\queue.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\debuglog.h
..\utils\cpp\container\inst_index.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp\container\list.h:	..\utils\cpp\smartptr.h ..\utils\cpp\cpputil.h
..\utils\cpp\showaccel.h:	..\utils\cpp\container\sorted_vector.h
infostate.cpp:	infostate.h ..\utils\debuglog.h
collectioninfocache.h:	..\include\config.h aplayable.h infobundle.h playableset.h ..\utils\cpp\mutex.h ..\utils\cpp\container\sorted_vector.h
waitinfo.cpp:	waitinfo.h ..\utils\debuglog.h
..\include\format.h:	..\include\plugin.h ..\utils\cpp\xstring.h
infobundle.h:	..\include\format.h ..\include\decoder_plug.h ..\utils\cpp\cpputil.h ..\utils\cpp\xstring.h ..\utils\strutils.h
gui.h:		controller.h songiterator.h playable.h pm123.rc.h ..\utils\cpp\smartptr.h ..\utils\cpp\event.h ..\utils\cpp\cpputil.h plugman.h
waitinfo.h:	aplayable.h ..\utils\cpp\event.h ..\utils\cpp\mutex.h
gui.cpp:	gui.h dialog.h filedlg.h properties.h configuration.h pm123.h pm123.rc.h 123_util.h glue.h skin.h docking.h infodialog.h playlistview.h playlistmanager.h inspector.h dependencyinfo.h loadhelper.h playlistmenu.h button95.h copyright.h decoder.h filter.h output.h visual.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\utils\cpp\showaccel.h
playable.cpp:	playable.h eventhandler.h properties.h decoder.h dependencyinfo.h location.h waitinfo.h ..\utils\strutils.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\eautils.h ..\utils\vdelegate.h ..\xio123\xio.h ..\utils\cpp\algorithm.h ..\utils\debuglog.h
playableinstance.h:	playableref.h
playableinstance.cpp:	playableinstance.h playable.h ..\utils\debuglog.h
songiterator.cpp:	songiterator.h ..\utils\debuglog.h
songiterator.h:	location.h playable.h
..\utils\cpp\pmutils.h:	..\utils\debuglog.h ..\utils\cpp\xstring.h ..\include\plugin.h
commandprocessor.cpp:	commandprocessor.h dialog.h gui.h location.h eventhandler.h 123_util.h pm123.h dependencyinfo.h copyright.h plugman.h ..\utils\fileutil.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
commandprocessor.h:	acommandprocessor.h configuration.h loadhelper.h controller.h songiterator.h plugman.h dependencyinfo.h ..\utils\cpp\cppvdelegate.h
xstring_api.h:	..\include\config.h ..\include\format.h ..\utils\cpp\xstring.h
xstring_api.cpp:	xstring_api.h ..\utils\interlocked.h ..\utils\cpp\xstring.h
decoder.cpp:	..\xio123\xio.h decoder.h controller.h glue.h configuration.h eventhandler.h proxyhelper.h ..\utils\cpp\cppvdelegate.h ..\utils\charset.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\cpp\url123.h pm123.h ..\utils\debuglog.h
decoder.h:	plugman.h ..\include\decoder_plug.h ..\utils\cpp\container\stringmap.h
filter.cpp:	filter.h infobundle.h eventhandler.h proxyhelper.h ..\utils\cpp\cppvdelegate.h ..\utils\debuglog.h
filter.h:	plugman.h ..\include\filter_plug.h ..\utils\vdelegate.h ..\utils\cpp\smartptr.h
output.h:	plugman.h ..\include\output_plug.h ..\include\filter_plug.h ..\include\decoder_plug.h ..\include\visual_plug.h ..\utils\cpp\smartptr.h
output.cpp:	output.h proxyhelper.h pm123.h ..\utils\cpp\cppvdelegate.h ..\utils\debuglog.h
visual.cpp:	visual.h glue.h eventhandler.h controller.h ..\include\decoder_plug.h ..\utils\debuglog.h
visual.h:	plugman.h ..\include\visual_plug.h ..\utils\cpp\smartptr.h
eventhandler.h:	..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\interlocked.h ..\utils\cpp\container\vector.h
proxyhelper.h:	..\include\config.h ..\include\decoder_plug.h
proxyhelper.cpp:	proxyhelper.h eventhandler.h copyright.h skin.h 123_util.h pm123.h controller.h playable.h ..\include\filter_plug.h ..\utils\charset.h ..\utils\debuglog.h
..\utils\cpp\vdelegate.h:	..\include\config.h ..\utils\vdelegate.h
configuration.h:	..\include\config.h ..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\cpp\event.h ..\utils\cpp\container\vector.h
configuration.cpp:	configuration.h eventhandler.h plugman.h visual.h pm123.h ..\utils\inimacro.h
eventhandler.cpp:	eventhandler.h ..\utils\cpp\cpputil.h
dependencyinfo.h:	aplayable.h ..\utils\cpp\event.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h
dependencyinfo.cpp:	dependencyinfo.h playableset.h playable.h ..\utils\debuglog.h
collectioninfocache.cpp:	collectioninfocache.h playable.h ..\utils\cpp\algorithm.h ..\utils\debuglog.h
..\utils\cpp\cppvdelegate.h:	..\utils\vdelegate.h
..\utils\cpp\windowbase.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\pmutils.h ..\utils\cpp\container\vector.h ..\utils\debuglog.h
..\gbm123\gbm.h:	..\include\config.h
acommandprocessor.cpp:	acommandprocessor.h
acommandprocessor.h:	..\include\config.h ..\utils\cpp\xstring.h
..\utils\cpp\dlgcontrols.h:	..\utils\cpp\xstring.h ..\utils\cpp\pmutils.h ..\utils\debuglog.h
randomplayable.cpp:	songiterator.h ..\utils\debuglog.h
