#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET   = pm123.exe
CFLAGS_USR = -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJCORE  = core\infobundle$(CO) core\infostate$(CO) core\aplayable$(CO) core\waitinfo$(CO) core\dependencyinfo$(CO)
OBJCORE  = $(OBJCORE) core\playableset$(CO) core\location$(CO) core\songiterator$(CO) core\collectioninfocache$(CO)
OBJCORE  = $(OBJCORE) core\playableref$(CO) core\playableinstance$(CO) core\playable$(CO)

OBJENGINE = engine\proxyhelper$(CO) engine\plugman$(CO) 
OBJENGINE = $(OBJENGINE) engine\decoder$(CO) engine\filter$(CO) engine\output$(CO) engine\visual$(CO)
OBJENGINE = $(OBJENGINE) engine\glue$(CO) engine\controller$(CO) engine\loadhelper$(CO)

OBJREMOTE = remote\commandprocessor$(CO) remote\acommandprocessor$(CO) remote\pipe$(CO)

OBJGUI   = gui\postmsginfo$(CO) gui\playlistmenu$(CO) gui\button95$(CO) gui\docking$(CO)
OBJGUI   = $(OBJGUI) gui\dialog$(CO) gui\filedlg$(CO) gui\properties$(CO) gui\infodialog$(CO)
OBJGUI   = $(OBJGUI) gui\playlistbase$(CO) gui\playlistview$(CO) gui\playlistmanager$(CO)
OBJGUI   = $(OBJGUI) gui\skin$(CO) gui\gui$(CO) gui\inspector$(CO)

OBJECTS  = eventhandler$(CO) configuration$(CO) 123_util$(CO) pm123$(CO)

DEF      = pm123$(LDEF_SUFFX)
RESOURCE = gui\pm123$(RES)
LIBS123  = core\pm123core$(LBO) engine\pm123engine$(LBO) remote\pm123remote$(LBO) gui\pm123gui$(LBO) pm123$(LBO)
LIBS     = $(LIBS123) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\gbm123\libgbm$(LBO)
STDLIBS  = $(LIBS_UNI)

# objects directly passed to the linker cause undefined symbols with gcc 3.3.5
core\pm123core$(LBO) : $(OBJCORE)
	if exist $@ del $@
	$(LB) $@ $(OBJCORE) $(LBENDL)

engine\pm123engine$(LBO) : $(OBJENGINE)
	if exist $@ del $@
	$(LB) $@ $(OBJENGINE) $(LBENDL)

remote\pm123remote$(LBO) : $(OBJREMOTE)
	if exist $@ del $@
	$(LB) $@ $(OBJREMOTE) $(LBENDL)

gui\pm123gui$(LBO) : $(OBJGUI)
	if exist $@ del $@
	$(LB) $@ $(OBJGUI) $(LBENDL)

pm123$(LBO) : $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

pm123.exe: $(LIBS) $(RESOURCE) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_PM) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	execmode -sp pm123.exe
	$(POSTLINKCMD)

#test.exe: test.obj $(DEF) $(LIBS)
#	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)test.obj $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)


clean: $(MDUMMY)
	-@del pm123.exe $(LIBS123) $(OBJCORE) $(OBJENGINE) $(OBJREMOTE) $(OBJGUI) $(OBJECTS) $(RESOURCE) 2> nul

todo.txt: $(MDUMMY)
	grep -i -A 5 TODO *.cpp *.h >todo.txt


# DO NOT DELETE

# include depenencies
123_util.cpp:	123_util.h configuration.h ..\utils\utilfct.h
123_util.h:	..\include\decoder_plug.h ..\utils\cpp\xstring.h ..\utils\cpp\url123.h
..\include\decoder_plug.h:	..\include\format.h ..\include\output_plug.h
..\include\format.h:	..\include\plugin.h ..\utils\cpp\xstring.h
..\include\plugin.h:	..\include\config.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
..\utils\cpp\xstring.h:	..\utils\cpp\smartptr.h
..\utils\cpp\smartptr.h:	..\include\config.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\cpp\mutex.h:	..\include\config.h ..\utils\interlocked.h ..\utils\debuglog.h
..\utils\interlocked.h:	..\include\config.h
..\utils\debuglog.h:	..\utils\errorstr.h
..\include\output_plug.h:	..\include\format.h
..\utils\cpp\url123.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h
..\utils\cpp\container\stringmap.h:	..\utils\cpp\xstring.h ..\utils\cpp\container\sorted_vector.h
..\utils\cpp\container\sorted_vector.h:	..\utils\cpp\smartptr.h ..\utils\cpp\algorithm.h ..\utils\cpp\container\vector.h
..\utils\cpp\algorithm.h:	..\utils\cpp\container\vector.h
..\utils\cpp\container\vector.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h
configuration.h:	..\include\config.h ..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\cpp\event.h ..\utils\cpp\container\vector.h
..\utils\cpp\event.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\debuglog.h
..\utils\utilfct.h:	..\include\config.h ..\utils\errorstr.h ..\utils\minmax.h ..\utils\strutils.h
configuration.cpp:	configuration.h eventhandler.h engine/plugman.h engine/visual.h pm123.h ..\utils\inimacro.h
eventhandler.h:	..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\interlocked.h ..\utils\cpp\container\vector.h
engine/plugman.h:	..\include\plugin.h ..\utils\cpp\event.h ..\utils\cpp\xstring.h ..\utils\cpp\container\vector.h
engine/visual.h:	..\include\visual_plug.h ..\utils\cpp\smartptr.h
..\include\visual_plug.h:	..\include\format.h ..\include\output_plug.h
pm123.h:	..\include\config.h ..\utils\cpp\xstring.h
..\utils\inimacro.h:	..\include\config.h
eventhandler.cpp:	eventhandler.h ..\utils\cpp\cpputil.h
pm123.cpp:	pm123.h eventhandler.h 123_util.h engine/plugman.h engine/controller.h engine/loadhelper.h gui/gui.h gui/dialog.h configuration.h remote/pipe.h remote/acommandprocessor.h ..\xio123\xio.h ..\utils\utilfct.h ..\utils\minmax.h ..\utils\debuglog.h
engine/controller.h:	..\utils\cpp\queue.h ..\utils\cpp\xstring.h ..\utils\cpp\event.h ..\utils\cpp\cpputil.h ..\utils\debuglog.h
..\utils\cpp\queue.h:	..\include\config.h ..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\debuglog.h
engine/loadhelper.h:	..\utils\cpp\smartptr.h
gui/gui.h:	..\utils\cpp\smartptr.h ..\utils\cpp\event.h ..\utils\cpp\cpputil.h ..\utils\cpp\windowbase.h
..\utils\cpp\windowbase.h:	..\utils\cpp\smartptr.h ..\utils\cpp\xstring.h ..\utils\cpp\pmutils.h ..\utils\cpp\dlgcontrols.h ..\utils\cpp\container\vector.h ..\utils\debuglog.h
..\utils\cpp\pmutils.h:	..\utils\debuglog.h ..\utils\cpp\xstring.h ..\include\plugin.h
..\utils\cpp\dlgcontrols.h:	..\utils\cpp\xstring.h ..\utils\cpp\pmutils.h ..\utils\debuglog.h
gui/dialog.h:	..\include\plugin.h ..\include\config.h ..\include\decoder_plug.h ..\utils\cpp\url123.h
remote/acommandprocessor.h:	..\include\config.h ..\utils\cpp\xstring.h
..\xio123\xio.h:	..\include\config.h
core\aplayable.cpp:	pm123.h configuration.h ..\utils\utilfct.h ..\utils\debuglog.h
core\collectioninfocache.cpp:	..\utils\cpp\algorithm.h ..\utils\debuglog.h
core\dependencyinfo.cpp:	..\utils\debuglog.h
core\infobundle.cpp:	123_util.h ..\utils\debuglog.h
core\infostate.cpp:	..\utils\debuglog.h
core\playable.cpp:	eventhandler.h ..\utils\strutils.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\eautils.h ..\utils\vdelegate.h ..\xio123\xio.h ..\utils\cpp\algorithm.h ..\utils\debuglog.h
..\utils\fileutil.h:	..\include\config.h
..\utils\vdelegate.h:	..\include\config.h
core\playableinstance.cpp:	..\utils\debuglog.h
core\playableref.cpp:	..\utils\debuglog.h
core\playableset.cpp:	..\utils\debuglog.h ..\utils\snprintf.h
core\songiterator.cpp:	..\utils\debuglog.h
core\waitinfo.cpp:	..\utils\debuglog.h
engine\controller.cpp:	configuration.h pm123.h ..\utils\inimacro.h ..\utils\minmax.h ..\utils\utilfct.h ..\utils\cpp\mutex.h ..\utils\cpp\xstring.h
engine\decoder.cpp:	..\xio123\xio.h configuration.h eventhandler.h ..\utils\cpp\cppvdelegate.h ..\utils\charset.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\cpp\url123.h pm123.h ..\utils\debuglog.h
..\utils\cpp\cppvdelegate.h:	..\utils\vdelegate.h
engine\filter.cpp:	eventhandler.h ..\utils\cpp\cppvdelegate.h ..\utils\debuglog.h
engine\glue.cpp:	configuration.h eventhandler.h ..\utils\debuglog.h
engine\loadhelper.cpp:	configuration.h
engine\output.cpp:	configuration.h pm123.h ..\utils\cpp\cppvdelegate.h ..\utils\debuglog.h
engine\plugman.cpp:	configuration.h eventhandler.h pm123.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\url123.h ..\utils\cpp\cppvdelegate.h ..\utils\fileutil.h ..\utils\debuglog.h
..\utils\cpp\container\inst_index.h:	..\utils\cpp\mutex.h ..\utils\cpp\smartptr.h ..\utils\cpp\container\sorted_vector.h
engine\proxyhelper.cpp:	eventhandler.h copyright.h pm123.h ..\include\filter_plug.h ..\utils\charset.h ..\utils\debuglog.h
..\include\filter_plug.h:	..\include\format.h ..\include\output_plug.h
engine\visual.cpp:	eventhandler.h ..\include\decoder_plug.h ..\utils\debuglog.h
gui\button95.cpp:	..\utils\utilfct.h ..\utils\debuglog.h
gui\dialog.cpp:	configuration.h 123_util.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\xio123\xio.h ..\utils\debuglog.h
gui\docking.cpp:	configuration.h eventhandler.h ..\utils\utilfct.h ..\utils\cpp\mutex.h
gui\filedlg.cpp:	configuration.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\wildcards.h ..\utils\cpp\xstring.h ..\utils\debuglog.h
gui\gui.cpp:	configuration.h pm123.h 123_util.h copyright.h ..\utils\utilfct.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\utils\cpp\showaccel.h
..\utils\cpp\showaccel.h:	..\utils\cpp\container\sorted_vector.h
gui\infodialog.cpp:	pm123.h configuration.h eventhandler.h ..\utils\utilfct.h ..\utils\cpp\container\inst_index.h ..\utils\cpp\pmutils.h ..\utils\cpp\dlgcontrols.h ..\utils\cpp\cppvdelegate.h
gui\inspector.cpp:	configuration.h pm123.h ..\utils\utilfct.h ..\utils\debuglog.h
gui\playlistbase.cpp:	pm123.h configuration.h 123_util.h ..\utils\utilfct.h ..\utils\cpp\url123.h ..\utils\cpp\pmutils.h ..\utils\debuglog.h
gui\playlistmanager.cpp:	configuration.h ..\utils\utilfct.h ..\utils\cpp\smartptr.h ..\utils\cpp\showaccel.h ..\utils\debuglog.h
gui\playlistmenu.cpp:	pm123.h ..\utils\cpp\cppvdelegate.h ..\utils\utilfct.h ..\utils\debuglog.h
gui\playlistview.cpp:	..\utils\cpp\showaccel.h ..\utils\utilfct.h ..\utils\debuglog.h
gui\postmsginfo.cpp:	..\utils\debuglog.h
gui\properties.cpp:	configuration.h eventhandler.h 123_util.h copyright.h pm123.h ..\utils\utilfct.h ..\utils\cpp\pmutils.h ..\utils\cpp\windowbase.h ..\utils\cpp\dlgcontrols.h
gui\skin.cpp:	configuration.h pm123.h eventhandler.h ..\gbm123\gbm.h ..\gbm123\gbmerr.h ..\gbm123\gbmht.h ..\utils\minmax.h ..\utils\strutils.h ..\utils\fileutil.h ..\utils\cpp\pmutils.h ..\utils\cpp\cpputil.h ..\utils\debuglog.h
..\gbm123\gbm.h:	..\include\config.h
remote\commandprocessor.cpp:	eventhandler.h 123_util.h pm123.h copyright.h ..\utils\fileutil.h ..\utils\cpp\xstring.h ..\utils\cpp\container\stringmap.h ..\utils\debuglog.h
remote\pipe.cpp:	configuration.h eventhandler.h
