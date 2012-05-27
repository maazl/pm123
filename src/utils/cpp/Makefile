#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..
TARGET   = cpputil$(LBO) cpputil_plugin$(LBO)
SRCFIL   = *.cpp container/*.cpp
CFLAGS_USR=-fcheck-new -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJECTS = showaccel$(CO) event$(CO) mutex$(CO) queue$(CO) priority_queue$(CO) algorithm$(CO)
OBJECTS = $(OBJECTS) xstring$(CO) url123$(CO) xstringbuilder$(CO)
OBJECTS = $(OBJECTS) container\vector$(CO) container\list$(CO) container\stringmap$(CO)
OBJECTS = $(OBJECTS) windowbase$(CO) dlgcontrols$(CO)

OBJECTS2 = xstring_alloc$(CO)
OBJECTS3 = xstring_alloc_plugin$(CO)

cpputil$(LBO) : $(OBJECTS) $(OBJECTS2) debugmem$(CO)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(OBJECTS2) $(LBENDL)

cpputil_plugin$(LBO) : $(OBJECTS) $(OBJECTS3) debugmem$(CO)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(OBJECTS3) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) xstring$(LBO) $(OBJECTS) $(OBJECTS2) $(OBJECTS3) 2> nul

OBJECTS4 = unittest$(CO)
LIBS4    = cpputil$(LBO)

test: unittest.exe
	unittest.exe

unittest.exe: $(OBJECTS4) $(LIBS3)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS4) $(LIBS3) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)

algorithm.cpp:	algorithm.h ..\debuglog.h
algorithm.h:	container\vector.h
container\vector.h:	mutex.h smartptr.h
mutex.h:	..\..\include\config.h ..\interlocked.h ..\debuglog.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\interlocked.h:	..\..\include\config.h
..\debuglog.h:	..\errorstr.h
smartptr.h:	..\..\include\config.h cpputil.h mutex.h ..\debuglog.h
event.cpp:	event.h cpputil.h interlockedptr.h ..\debuglog.h
event.h:	..\..\include\config.h mutex.h ..\debuglog.h
mutex.cpp:	mutex.h ..\debuglog.h
queue.cpp:	queue.h ..\debuglog.h
queue.h:	..\..\include\config.h mutex.h smartptr.h ..\debuglog.h
unittest.cpp:	..\..\include\config.h event.h
url123.cpp:	url123.h ..\strutils.h ..\fileutil.h ..\debuglog.h
url123.h:	xstring.h container\stringmap.h
xstring.h:	smartptr.h
container\stringmap.h:	xstring.h container\sorted_vector.h
container\sorted_vector.h:	smartptr.h algorithm.h container\vector.h
xstringbuilder.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
priority_queue.cpp:	queue.h ..\debuglog.h ..\snprintf.h
showaccel.cpp:	showaccel.h ..\snprintf.h
showaccel.h:	container\sorted_vector.h
container/list.h:	smartptr.h cpputil.h
container/vector.cpp:	container/vector.h cpputil.h ..\debuglog.h
container/vector.h:	mutex.h smartptr.h
container/stringmap.cpp:	container/stringmap.h
container/stringmap.h:	xstring.h container\sorted_vector.h
container/iterator.cpp:	container\iterator.h
container\iterator.h:	container\vector.h
debugmem.cpp:	..\debuglog.h
interlockedptr.h:	..\interlocked.h
pmutils.cpp:	pmutils.h
pmutils.h:	..\debuglog.h xstring.h ..\..\include\plugin.h
..\..\include\plugin.h:	..\..\include\config.h
smartptr.cpp:	mutex.h smartptr.h ..\debuglog.h
container/list.cpp:	container/list.h ..\debuglog.h
..\fileutil.h:	..\..\include\config.h
windowbase.cpp:	..\utilfct.h windowbase.h smartptr.h mutex.h ..\debuglog.h
..\utilfct.h:	..\..\include\config.h ..\errorstr.h ..\minmax.h ..\strutils.h
windowbase.h:	smartptr.h xstring.h pmutils.h dlgcontrols.h container\vector.h ..\debuglog.h
xstring_alloc.cpp:	xstring.h ..\debuglog.h
xstring_alloc_plugin.cpp:	xstring.h ..\..\include\plugin.h ..\debuglog.h
xstring.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
dlgcontrols.cpp:	dlgcontrols.h ..\debuglog.h
dlgcontrols.h:	xstring.h pmutils.h ..\debuglog.h
