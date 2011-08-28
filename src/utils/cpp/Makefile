#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..
TARGET   = all2
SRCFIL   = *.cpp container/*.cpp

!include $(TOPDIR)\config\makerules

all2: cpputil$(LBO) xstring$(LBO) $(MDUMMY)

OBJECTS = showaccel$(CO) event$(CO) mutex$(CO) queue$(CO) priority_queue$(CO) algorithm$(CO)
OBJECTS = $(OBJECTS) container\vector$(CO) container\list$(CO)
# pmutils$(CO)
#OBJECTS = $(OBJECTS) PM\PMMenu$(CO)

OBJECTS2 = xstring$(CO) xstringbuilder$(CO) url123$(CO) container\stringmap$(CO)

cpputil$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

xstring$(LBO): $(OBJECTS2)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS2) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) xstring$(LBO) $(OBJECTS) $(OBJECTS2) 2> nul

OBJECTS3 = unittest$(CO)
LIBS3    = cpputil$(LBO)

test: unittest.exe
	unittest.exe

unittest.exe: $(OBJECTS3) $(LIBS3)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS3) $(LIBS3) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)

# Keep wmake happy
container\vector$(CO) : container\vector.cpp
	$(CC) $(CFLAGS) -c $[@ -fo=$@
container\list$(CO) : container\list.cpp
	$(CC) $(CFLAGS) -c $[@ -fo=$@
container\stringmap$(CO) : container\stringmap.cpp
	$(CC) $(CFLAGS) -c $[@ -fo=$@

algorithm.cpp:	algorithm.h ..\debuglog.h
algorithm.h:	container\vector.h
container\vector.h:	mutex.h smartptr.h
mutex.h:	..\..\include\config.h ..\interlocked.h ..\debuglog.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\interlocked.h:	..\..\include\config.h
..\debuglog.h:	..\errorstr.h
smartptr.h:	..\..\include\config.h cpputil.h mutex.h ..\debuglog.h
event.cpp:	event.h cpputil.h ..\debuglog.h
event.h:	..\..\include\config.h mutex.h ..\debuglog.h
mutex.cpp:	mutex.h ..\debuglog.h
queue.cpp:	queue.h ..\debuglog.h
queue.h:	..\..\include\config.h mutex.h smartptr.h ..\debuglog.h
unittest.cpp:	..\..\include\config.h event.h
url123.cpp:	url123.h ..\strutils.h ..\debuglog.h
url123.h:	xstring.h container\stringmap.h
xstring.h:	smartptr.h
container\stringmap.h:	xstring.h container\sorted_vector.h
container\sorted_vector.h:	smartptr.h algorithm.h container\vector.h
xstring.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
xstringbuilder.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
priority_queue.cpp:	queue.h ..\debuglog.h ..\snprintf.h
showaccel.cpp:	showaccel.h ..\snprintf.h
showaccel.h:	container\sorted_vector.h
container/list.cpp:	container/list.h ..\debuglog.h
container/list.h:	smartptr.h cpputil.h
container/vector.cpp:	container/vector.h cpputil.h ..\debuglog.h
container/vector.h:	mutex.h smartptr.h
container/stringmap.cpp:	container/stringmap.h
container/stringmap.h:	xstring.h container\sorted_vector.h
container/iterator.cpp:	container\iterator.h
container\iterator.h:	container\vector.h
