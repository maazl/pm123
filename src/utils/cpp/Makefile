#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;.
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: cpputil$(LBO) $(MDUMMY)

OBJECTS = event$(CO) mutex$(CO) container$(CO) queue$(CO)
OBJECTS = $(OBJECTS) xstring$(CO) smartptr$(CO)

cpputil$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) $(OBJECTS) 2> nul

event.cpp:     event.h ..\..\utils\debuglog.h
mutex.cpp:     mutex.h ..\..\utils\debuglog.h
smartptr.cpp:  mutex.h smartptr.h ..\..\utils\debuglog.h
simplestr.cpp: simplestr.h
xstring.cpp:   xstring.h ..\..\utils\snprintf.h ..\..\utils\debuglog.h
xstring.h:     ..\..\utils\cpp/smartptr.h
queue.cpp:     queue.h ..\..\utils\debuglog.h
queue.h:       ..\..\include\config.h ..\..\utils\cpp/mutex.h ..\..\utils\cpp/smartptr.h
container.cpp: container.h ..\..\utils\debuglog.h
container.h:   ..\..\utils\cpp/mutex.h ..\..\utils\cpp/smartptr.h

OBJECTS = unittest$(CO)
LIBS    = cpputil$(LBO)

test: unittest.exe
	unittest.exe

unittest.exe: $(OBJECTS) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)
event.h:	..\..\utils\cpp/mutex.h ..\..\include\config.h ..\..\utils\debuglog.h
mutex.h:	..\..\include\config.h
simplestr.h:	..\..\utils\cpp/smartptr.h
smartptr.h:	..\..\include\config.h ..\..\utils\debuglog.h
