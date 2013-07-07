#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..
TARGET   = cpputil$(LBO) cpputil_plugin$(LBO)
SRCFIL   = *.cpp
CFLAGS_USR=-fcheck-new -DPM123_CORE

!include $(TOPDIR)\config\makerules

OBJECTS = showaccel$(CO) event$(CO) mutex$(CO) queue$(CO) priority_queue$(CO) algorithm$(CO)
OBJECTS = $(OBJECTS) xstring$(CO) xstringconst$(CO) url123$(CO) xstringbuilder$(CO)
OBJECTS = $(OBJECTS) container\vector$(CO) container\list$(CO) container\stringset$(CO) container\stringmap$(CO)
OBJECTS = $(OBJECTS) container\btree$(CO)
OBJECTS = $(OBJECTS) pmutils_dnd$(CO) windowbase$(CO) dlgcontrols$(CO)

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

LIBTEST = cpputil$(LBO) ..\utilfct$(LBO)

test: test\test_int_ptr.exe test\test_btree.exe
	test\test_int_ptr.exe
	test\test_btree.exe

test\test_int_ptr.exe: test\test_int_ptr$(CO) $(LIBTEST)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)test\test_int_ptr$(CO) $(LIBTEST) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)

test\test_btree.exe: test\test_btree$(CO) $(LIBTEST)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)test\test_btree$(CO) $(LIBTEST) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)

algorithm.cpp:	algorithm.h ..\debuglog.h
algorithm.h:	container\vector.h
debugmem.cpp:	..\debuglog.h
dlgcontrols.cpp:	dlgcontrols.h ..\debuglog.h
dlgcontrols.h:	xstring.h ..\cpp\pmutils.h ..\debuglog.h
xstring.h:	smartptr.h
smartptr.h:	..\..\include\config.h cpputil.h mutex.h ..\debuglog.h
mutex.h:	..\..\include\config.h ..\interlocked.h ..\debuglog.h
pmutils.h:	..\debuglog.h xstring.h ..\..\include\plugin.h
event.cpp:	event.h cpputil.h interlockedptr.h ..\debuglog.h
event.h:	..\..\include\config.h mutex.h ..\debuglog.h
interlockedptr.h:	..\interlocked.h
mutex.cpp:	mutex.h ..\debuglog.h
pmutils_dnd.cpp:	pmutils.h
priority_queue.cpp:	queue.h ..\debuglog.h ..\snprintf.h
queue.h:	..\..\include\config.h mutex.h smartptr.h ..\debuglog.h
queue.cpp:	queue.h ..\debuglog.h
showaccel.cpp:	showaccel.h ..\snprintf.h
showaccel.h:	container\sorted_vector.h
smartptr.cpp:	mutex.h smartptr.h ..\debuglog.h
url123.cpp:	url123.h ..\strutils.h ..\fileutil.h ..\debuglog.h
url123.h:	xstring.h container\stringmap.h cpputil.h
windowbase.cpp:	..\utilfct.h windowbase.h smartptr.h mutex.h ..\debuglog.h
windowbase.h:	smartptr.h xstring.h pmutils.h dlgcontrols.h container\vector.h ..\debuglog.h
xstring.cpp:	xstring.h container\btree.h ..\snprintf.h ..\debuglog.h
xstringbuilder.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
xstringconst.cpp:	xstring.h ..\interlocked.h ..\debuglog.h
xstring_alloc.cpp:	xstring.h ..\debuglog.h
xstring_alloc_plugin.cpp:	xstring.h ..\..\include\plugin.h ..\debuglog.h
container\btree.cpp:	container\btree.h
container\list.cpp:	container\list.h
container\stringmap.cpp:	container\stringmap.h
container\vector.cpp:	container\vector.h cpputil.h
test\test_btree.cpp:	container\btree.h
container\stringset.cpp:	container\stringset.h
