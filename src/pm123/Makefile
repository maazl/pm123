#
#  pm123.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET   = pm123.exe
CFLAGS_USR = -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJCORE  = core\infobundle$(CO) core\infostate$(CO) core\aplayable$(CO) core\waitinfo$(CO) core\dependencyinfo$(CO) core\job$(CO)
OBJCORE  = $(OBJCORE) core\playableset$(CO) core\location$(CO) core\songiterator$(CO) core\collectioninfocache$(CO)
OBJCORE  = $(OBJCORE) core\playableref$(CO) core\playableinstance$(CO) core\playable$(CO)

OBJENGINE = engine\proxyhelper$(CO) engine\plugman$(CO) 
OBJENGINE = $(OBJENGINE) engine\decoder$(CO) engine\filter$(CO) engine\output$(CO) engine\visual$(CO)
OBJENGINE = $(OBJENGINE) engine\glue$(CO) engine\controller$(CO) engine\loadhelper$(CO)

OBJREMOTE = remote\commandprocessor$(CO) remote\acommandprocessor$(CO) remote\pipe$(CO)

OBJGUI   = gui\postmsginfo$(CO) gui\playlistmenu$(CO) gui\button95$(CO) gui\docking$(CO)
OBJGUI   = $(OBJGUI) gui\dialog$(CO) gui\filedlg$(CO) gui\properties$(CO) gui\infodialog$(CO) gui\metawritedlg$(CO)
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
..\include\output_plug.h:	..\include\format.h
configuration.h:	..\include\config.h ..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\cpputil.h ..\utils\cpp\mutex.h ..\utils\cpp\event.h ..\utils\cpp\container\vector.h
..\utils\utilfct.h:	..\include\config.h ..\utils\errorstr.h ..\utils\minmax.h ..\utils\strutils.h
configuration.cpp:	configuration.h eventhandler.h engine\plugman.h engine\visual.h pm123.h ..\utils\inimacro.h
eventhandler.h:	..\include\plugin.h ..\utils\cpp\xstring.h ..\utils\cpp\smartptr.h ..\utils\cpp\mutex.h ..\utils\interlocked.h ..\utils\cpp\container\vector.h
..\utils\interlocked.h:	..\include\config.h
engine\visual.h:	engine\plugman.h
pm123.h:	..\include\config.h ..\utils\cpp\xstring.h
..\utils\inimacro.h:	..\include\config.h
eventhandler.cpp:	eventhandler.h ..\utils\cpp\cpputil.h
pm123.cpp:	pm123.h eventhandler.h 123_util.h engine\plugman.h engine\controller.h engine\loadhelper.h gui\gui.h gui\dialog.h configuration.h remote\pipe.h remote\acommandprocessor.h ..\xio123\xio.h ..\utils\utilfct.h ..\utils\minmax.h ..\utils\debuglog.h
engine\controller.h:	core\aplayable.h
core\aplayable.h:	core\infostate.h
core\infostate.h:	core\infobundle.h
engine\loadhelper.h:	engine\controller.h
gui\gui.h:	engine\controller.h core\playable.h gui\pm123.rc.h
core\playable.h:	core\aplayable.h core\playableinstance.h core\collectioninfocache.h
core\playableinstance.h:	core\playableref.h
core\playableref.h:	core\aplayable.h core\collectioninfocache.h core\location.h
core\collectioninfocache.h:	core\aplayable.h core\infobundle.h core\playableset.h
core\location.h:	core\aplayable.h
gui\dialog.h:	gui\filedlg.h
..\xio123\xio.h:	..\include\config.h ..\utils\cpp\xstring.h
..\utils\debuglog.h:	..\utils\errorstr.h
core\aplayable.cpp:	core\aplayable.h core\playable.h core\playableset.h core\waitinfo.h core\dependencyinfo.h core\job.h core\location.h pm123.h configuration.h
core\waitinfo.h:	core\aplayable.h
core\dependencyinfo.h:	core\aplayable.h
core\collectioninfocache.cpp:	core\collectioninfocache.h core\playable.h
core\dependencyinfo.cpp:	core\dependencyinfo.h core\playableset.h core\playable.h
core\infobundle.cpp:	123_util.h core\infobundle.h core\playableset.h core\playable.h
core\infostate.cpp:	core\infostate.h
core\location.cpp:	core\location.h core\playable.h core\job.h
core\playable.cpp:	core\playable.h eventhandler.h engine\decoder.h core\job.h core\location.h core\waitinfo.h
engine\decoder.h:	engine\plugman.h
core\playableinstance.cpp:	core\playableinstance.h core\playable.h
core\playableref.cpp:	core\playableref.h core\playable.h core\location.h core\job.h
core\playableset.cpp:	core\playableset.h core\playable.h
core\songiterator.cpp:	core\songiterator.h core\playable.h core\job.h
core\songiterator.h:	core\location.h core\playable.h
core\waitinfo.cpp:	core\waitinfo.h
engine\controller.cpp:	engine\controller.h core\job.h core\songiterator.h configuration.h engine\decoder.h engine\glue.h pm123.h gui\gui.h
engine\glue.h:	core\playable.h
engine\decoder.cpp:	engine\decoder.h engine\controller.h engine\glue.h configuration.h eventhandler.h engine\proxyhelper.h pm123.h
engine\filter.cpp:	engine\filter.h core\infobundle.h eventhandler.h engine\proxyhelper.h
engine\filter.h:	engine\plugman.h
engine\glue.cpp:	engine\glue.h engine\decoder.h engine\filter.h engine\output.h configuration.h eventhandler.h
engine\output.h:	engine\plugman.h
engine\loadhelper.cpp:	engine\loadhelper.h configuration.h gui\gui.h core\job.h
engine\output.cpp:	engine\output.h engine\proxyhelper.h configuration.h pm123.h
engine\plugman.cpp:	engine\plugman.h engine\decoder.h engine\filter.h engine\output.h engine\visual.h gui\filedlg.h core\playable.h remote\acommandprocessor.h configuration.h eventhandler.h engine\proxyhelper.h pm123.h
engine\proxyhelper.cpp:	engine\proxyhelper.h eventhandler.h copyright.h pm123.h gui\gui.h gui\skin.h engine\controller.h core\playable.h
engine\visual.cpp:	engine\visual.h engine\glue.h eventhandler.h engine\controller.h
gui\button95.cpp:	gui\button95.h
gui\dialog.cpp:	gui\dialog.h configuration.h gui\gui.h 123_util.h gui\pm123.rc.h
gui\docking.cpp:	gui\docking.h configuration.h eventhandler.h
gui\filedlg.cpp:	gui\filedlg.h gui\pm123.rc.h configuration.h gui\dialog.h engine\plugman.h engine\decoder.h
gui\gui.cpp:	gui\gui.h core\songiterator.h core\job.h engine\visual.h engine\decoder.h engine\filter.h engine\output.h engine\loadhelper.h gui\dialog.h gui\filedlg.h gui\properties.h configuration.h pm123.h gui\pm123.rc.h 123_util.h gui\skin.h gui\docking.h gui\infodialog.h gui\playlistview.h gui\playlistmanager.h gui\inspector.h gui\postmsginfo.h gui\playlistmenu.h gui\button95.h copyright.h
gui\infodialog.h:	gui\ainfodialog.h eventhandler.h gui\pm123.rc.h
gui\playlistview.h:	gui\playlistbase.h
gui\playlistbase.h:	core\playableinstance.h core\playable.h engine\controller.h gui\ainfodialog.h
gui\playlistmanager.h:	gui\playlistbase.h
gui\postmsginfo.h:	core\dependencyinfo.h
gui\playlistmenu.h:	core\aplayable.h core\location.h
gui\infodialog.cpp:	gui\infodialog.h gui\metawritedlg.h core\job.h pm123.h gui\gui.h gui\pm123.rc.h configuration.h
gui\inspector.cpp:	gui\inspector.h gui\pm123.rc.h configuration.h core\playable.h core\dependencyinfo.h engine\controller.h pm123.h
gui\playlistbase.cpp:	gui\playlistbase.h core\job.h engine\plugman.h engine\loadhelper.h gui\playlistmanager.h gui\playlistview.h gui\playlistmenu.h gui\infodialog.h gui\inspector.h gui\postmsginfo.h pm123.h gui\gui.h gui\dialog.h configuration.h 123_util.h gui\pm123.rc.h gui\docking.h
gui\playlistmanager.cpp:	gui\playlistmanager.h core\playable.h engine\decoder.h gui\playlistmenu.h gui\gui.h configuration.h gui\pm123.rc.h
gui\playlistmenu.cpp:	pm123.h gui\pm123.rc.h gui\playlistmenu.h core\playable.h core\job.h
gui\playlistview.cpp:	gui\playlistview.h core\playable.h engine\decoder.h gui\playlistmenu.h gui\gui.h gui\dialog.h gui\metawritedlg.h gui\pm123.rc.h
gui\postmsginfo.cpp:	gui\postmsginfo.h
gui\properties.cpp:	gui\properties.h configuration.h gui\pm123.rc.h eventhandler.h gui\dialog.h gui\gui.h 123_util.h copyright.h engine\plugman.h engine\decoder.h engine\visual.h pm123.h
gui\skin.cpp:	gui\skin.h configuration.h engine\visual.h pm123.h eventhandler.h gui\pm123.rc.h gui\button95.h gui\dialog.h
remote\acommandprocessor.cpp:	remote\acommandprocessor.h remote\commandprocessor.h
remote\commandprocessor.h:	remote\acommandprocessor.h configuration.h core\songiterator.h core\job.h engine\controller.h engine\plugman.h
remote\pipe.cpp:	remote\pipe.h remote\acommandprocessor.h configuration.h eventhandler.h
core\job.h:	core\dependencyinfo.h
core\job.cpp:	core\job.h core\playableset.h
remote\commandprocessor.cpp:	remote\commandprocessor.h core\location.h core\job.h engine\plugman.h engine\loadhelper.h gui\dialog.h gui\gui.h eventhandler.h 123_util.h pm123.h copyright.h
gui\ainfodialog.h:	core\playable.h
gui\metawritedlg.h:	core\aplayable.h pm123.h
gui\metawritedlg.cpp:	gui\metawritedlg.h gui\pm123.rc.h core\playable.h eventhandler.h
