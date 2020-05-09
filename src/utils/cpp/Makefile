#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..
TARGET   = cpputil$(LBO) cpputil_plugin$(LBO)
SRCFIL   = *.cpp
CFLAGS_USR=-fcheck-new -DPM123_CORE

include $(TOPDIR)\config\makerules

OBJECTS = showaccel$(CO) event$(CO) mutex$(CO) queue$(CO) priority_queue$(CO) algorithm$(CO) \
	xstring$(CO) xstringconst$(CO) xstringbuilder$(CO) url123$(CO) directory$(CO) \
	container\vector$(CO) container\list$(CO) container\stringset$(CO) container\stringmap$(CO) \
	container\btree$(CO) \
	pmutils_dnd$(CO) windowbase$(CO) dlgcontrols$(CO)

OBJECTS2 = xstring_alloc$(CO)
OBJECTS3 = xstring_alloc_plugin$(CO)

cpputil$(LBO) : $(OBJECTS) $(OBJECTS2) debugmem$(CO)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(OBJECTS2) $(LBENDL)

cpputil_plugin$(LBO) : $(OBJECTS) $(OBJECTS3) debugmem$(CO)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(OBJECTS3) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) xstring$(LBO) $(OBJECTS) $(OBJECTS2) $(OBJECTS3) debugmem$(CO) 2> nul

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

algorithm.o:	algorithm.cpp ..\debuglog.h
debugmem.o:	debugmem.cpp 
directory.o:	directory.cpp ..\cpp\container\stringset.h ..\errorstr.h
dlgcontrols.o:	dlgcontrols.cpp ..\debuglog.h
event.o:	event.cpp cpputil.h ..\debuglog.h
mutex.o:	mutex.cpp ..\debuglog.h
pmutils_dnd.o:	pmutils_dnd.cpp 
priority_queue.o:	priority_queue.cpp ..\debuglog.h 
queue.o:	queue.cpp ..\debuglog.h
showaccel.o:	showaccel.cpp ..\snprintf.h
url123.o:	url123.cpp ..\strutils.h ..\debuglog.h
windowbase.o:	windowbase.cpp windowbase.h ..\cpp\xstring.h  ..\cpp\dlgcontrols.h ..\cpp\pmutils.h ..\vdelegate.h ..\cpp\mutex.h ..\interlocked.h
xstring.o:	xstring.cpp container\btree.h ..\debuglog.h
xstring_alloc.o:	xstring_alloc.cpp ..\debuglog.h
xstring_alloc_plugin.o:	xstring_alloc_plugin.cpp ..\..\include\plugin.h 
xstringbuilder.o:	xstringbuilder.cpp ..\snprintf.h 
xstringconst.o:	xstringconst.cpp ..\interlocked.h 
container\btree.o:	container\btree.cpp cpputil.h 
container\iterator.o:	container\iterator.cpp 
container\list.o:	container\list.cpp 
container\stringmap.o:	container\stringmap.cpp 
container\stringset.o:	container\stringset.cpp 
test\test_btree.o:	test\test_btree.cpp 
test\test_int_ptr.o:	test\test_int_ptr.cpp
container\vector.o:	container\vector.cpp cpputil.h
