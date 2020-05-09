#
#  pm123.exe makefile
#

TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\xio123
TARGET  = pm123.exe
CFLAGS_USR = -DPM123_CORE

include $(TOPDIR)\config\makerules

OBJCORE = core\infobundle$(CO) core\infostate$(CO) core\aplayable$(CO) core\waitinfo$(CO) core\dependencyinfo$(CO) core\job$(CO) \
	core\playableset$(CO) core\location$(CO) core\songiterator$(CO) core\collectioninfocache$(CO) \
	core\playableref$(CO) core\playableinstance$(CO) core\playable$(CO)

OBJENGINE = engine\proxyhelper$(CO) engine\plugman$(CO) \
	engine\decoder$(CO) engine\filter$(CO) engine\output$(CO) engine\visual$(CO) \
	engine\glue$(CO) engine\controller$(CO) engine\loadhelper$(CO)

OBJREMOTE = remote\commandprocessor$(CO) remote\acommandprocessor$(CO) remote\pipe$(CO)

OBJGUI = gui\postmsginfo$(CO) gui\playlistmenu$(CO) gui\button95$(CO) gui\docking$(CO) \
	gui\dialog$(CO) gui\filedlg$(CO) gui\properties$(CO) gui\infodialog$(CO) gui\metawritedlg$(CO) \
	gui\playlistbase$(CO) gui\playlistview$(CO) gui\playlistmanager$(CO) \
	gui\skin$(CO) gui\gui$(CO) gui\inspector$(CO)

OBJECTS = eventhandler$(CO) configuration$(CO) 123_util$(CO) pm123$(CO)

DEF     = pm123$(LDEF_SUFFX)
RESOURCE= gui\pm123$(RES)
LIBS123 = core\pm123core$(LBO) engine\pm123engine$(LBO) remote\pm123remote$(LBO) gui\pm123gui$(LBO) pm123$(LBO)
LIBS    = $(LIBS123) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\xio123\xio123$(LBI) \
	$(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\gbm123\libgbm$(LBO)
STDLIBS = $(LIBS_UNI)

# objects directly passed to the linker cause undefined symbols with gcc 3.3.5
core\pm123core$(LBO) : $(OBJCORE)
	-@del $@ 2>nul
	$(LB) $@ $(OBJCORE) $(LBENDL)

engine\pm123engine$(LBO) : $(OBJENGINE)
	-@del $@ 2>nul
	$(LB) $@ $(OBJENGINE) $(LBENDL)

remote\pm123remote$(LBO) : $(OBJREMOTE)
	-@del $@ 2>nul
	$(LB) $@ $(OBJREMOTE) $(LBENDL)

gui\pm123gui$(LBO) : $(OBJGUI)
	-@del $@ 2>nul
	$(LB) $@ $(OBJGUI) $(LBENDL)

pm123$(LBO) : $(OBJECTS)
	-@del $@ 2>nul
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
123_util.o:	123_util.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h
configuration.o:	configuration.cpp eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h engine\visual.h  ..\utils\inimacro.h
eventhandler.o:	eventhandler.cpp ..\utils\cpp\cpputil.h
pm123.o:	pm123.cpp eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h engine\plugman.h engine\loadhelper.h  gui\dialog.h remote\pipe.h ..\xio123\xio.h ..\utils\minmax.h
core\aplayable.o:	core\aplayable.cpp core\playable.h core\playableinstance.h  core\waitinfo.h core\job.h pm123.h ..\utils\cpp\xstring.h
core\collectioninfocache.o:	core\collectioninfocache.cpp core\playable.h core\playableinstance.h 
core\dependencyinfo.o:	core\dependencyinfo.cpp core\playableset.h 
core\infobundle.o:	core\infobundle.cpp core\infobundle.h core\playable.h core\playableinstance.h 
core\infostate.o:	core\infostate.cpp 
core\job.o:	core\job.cpp core\playableset.h
core\location.o:	core\location.cpp core\playable.h core\playableinstance.h 
core\playableinstance.o:	core\playableinstance.cpp core\playable.h core\playableinstance.h 
core\playableref.o:	core\playableref.cpp core\playable.h core\playableinstance.h  core\job.h
core\playableset.o:	core\playableset.cpp core\playable.h core\playableinstance.h 
core\songiterator.o:	core\songiterator.cpp core\playable.h core\playableinstance.h 
core\waitinfo.o:	core\waitinfo.cpp 
core\playable.o:	core\playable.cpp eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h core\job.h  core\waitinfo.h
engine\controller.o:	engine\controller.cpp core\job.h  configuration.h ..\include\plugin.h ..\utils\cpp\cpputil.h ..\utils\cpp\event.h engine\glue.h gui\gui.h core\playable.h core\playableinstance.h
engine\decoder.o:	engine\decoder.cpp engine\controller.h  configuration.h ..\include\plugin.h ..\utils\cpp\cpputil.h ..\utils\cpp\event.h engine\proxyhelper.h
engine\filter.o:	engine\filter.cpp core\infobundle.h engine\proxyhelper.h
engine\glue.o:	engine\glue.cpp engine\decoder.h  engine\output.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h
engine\loadhelper.o:	engine\loadhelper.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h core\job.h
engine\output.o:	engine\output.cpp engine\proxyhelper.h pm123.h ..\utils\cpp\xstring.h
engine\plugman.o:	engine\plugman.cpp engine\decoder.h  engine\output.h gui\filedlg.h remote\acommandprocessor.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h pm123.h
engine\proxyhelper.o:	engine\proxyhelper.cpp eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h pm123.h gui\skin.h core\playable.h core\playableinstance.h 
engine\visual.o:	engine\visual.cpp engine\glue.h  engine\controller.h
gui\button95.o:	gui\button95.cpp 
gui\dialog.o:	gui\dialog.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h 123_util.h ..\utils\cpp\xstring.h
gui\docking.o:	gui\docking.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h
gui\filedlg.o:	gui\filedlg.cpp gui\pm123.rc.h gui\dialog.h  engine\decoder.h
gui\gui.o:	gui\gui.cpp core\songiterator.h core\playable.h core\playableinstance.h  engine\visual.h engine\filter.h engine\loadhelper.h gui\filedlg.h configuration.h ..\include\plugin.h ..\utils\cpp\cpputil.h ..\utils\cpp\event.h gui\pm123.rc.h gui\skin.h gui\infodialog.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h gui\playlistmanager.h gui\postmsginfo.h gui\button95.h
gui\infodialog.o:	gui\infodialog.cpp gui\metawritedlg.h pm123.h ..\utils\cpp\xstring.h gui\pm123.rc.h 
gui\inspector.o:	gui\inspector.cpp gui\pm123.rc.h core\playable.h core\playableinstance.h  engine\controller.h
gui\playlistbase.o:	gui\playlistbase.cpp core\job.h  engine\loadhelper.h gui\playlistview.h gui\infodialog.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h gui\postmsginfo.h gui\gui.h core\playable.h core\playableinstance.h configuration.h ..\include\plugin.h ..\utils\cpp\cpputil.h ..\utils\cpp\event.h gui\pm123.rc.h
gui\playlistmanager.o:	gui\playlistmanager.cpp core\playable.h core\playableinstance.h  gui\playlistmenu.h core\location.h configuration.h ..\include\plugin.h ..\utils\cpp\cpputil.h ..\utils\cpp\event.h
gui\playlistmenu.o:	gui\playlistmenu.cpp gui\pm123.rc.h core\playable.h core\playableinstance.h 
gui\playlistview.o:	gui\playlistview.cpp core\playable.h core\playableinstance.h  gui\playlistmenu.h core\location.h gui\dialog.h gui\pm123.rc.h
gui\postmsginfo.o:	gui\postmsginfo.cpp 
gui\properties.o:	gui\properties.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h gui\gui.h core\playable.h core\playableinstance.h copyright.h engine\decoder.h pm123.h
gui\skin.o:	gui\skin.cpp configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h pm123.h ..\utils\cpp\xstring.h gui\pm123.rc.h gui\dialog.h
gui\metawritedlg.o:	gui\metawritedlg.cpp gui\pm123.rc.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h
remote\acommandprocessor.o:	remote\acommandprocessor.cpp remote\commandprocessor.h configuration.h ..\include\plugin.h  ..\utils\cpp\cpputil.h ..\utils\cpp\event.h core\job.h engine\plugman.h
remote\commandprocessor.o:	remote\commandprocessor.cpp core\location.h  engine\plugman.h gui\dialog.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h pm123.h
remote\pipe.o:	remote\pipe.cpp remote\acommandprocessor.h eventhandler.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h ..\utils\cpp\container\vector.h
