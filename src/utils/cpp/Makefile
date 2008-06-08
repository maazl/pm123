#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: cpputil$(LBO) $(MDUMMY)

OBJECTS = showaccel$(CO) event$(CO) mutex$(CO) container$(CO) queue$(CO)
OBJECTS = $(OBJECTS) xstring$(CO) url123$(CO) stringmap$(CO) smartptr$(CO)

cpputil$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) $(OBJECTS) 2> nul

OBJECTS = unittest$(CO)
LIBS    = cpputil$(LBO)

test: unittest.exe
	unittest.exe

unittest.exe: $(OBJECTS) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)

container.cpp:	container.h cpputil.h ..\debuglog.h
container.h:	..\cpp/mutex.h ..\cpp/smartptr.h
event.cpp:	event.h ..\debuglog.h
event.h:	..\..\include\config.h ..\cpp/mutex.h ..\debuglog.h
mutex.cpp:	mutex.h ..\debuglog.h
mutex.h:	..\..\include\config.h
queue.cpp:	queue.h ..\debuglog.h
queue.h:	..\..\include\config.h ..\cpp/mutex.h ..\cpp/smartptr.h ..\debuglog.h
simplestr.cpp:	simplestr.h
simplestr.h:	..\cpp/smartptr.h
smartptr.cpp:	mutex.h smartptr.h ..\debuglog.h
smartptr.h:	..\..\include\config.h ..\cpp/cpputil.h ..\debuglog.h
stringmap.cpp:	stringmap.h
stringmap.h:	..\cpp/xstring.h ..\cpp/container.h
unittest.cpp:	..\..\include\config.h event.h
url123.cpp:	url123.h ..\strutils.h ..\debuglog.h
url123.h:	..\cpp/xstring.h ..\cpp/stringmap.h
xstring.cpp:	xstring.h ..\snprintf.h ..\debuglog.h
xstring.h:	..\cpp/smartptr.h
showaccel.cpp:	showaccel.h ..\snprintf.h
showaccel.h:	..\cpp/container.h
