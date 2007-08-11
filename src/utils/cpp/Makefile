#
#  utilfct library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;.

!include $(TOPDIR)\config\makerules

all: cpputil$(LBO) $(MDUMMY)

OBJECTS = event$(CO) mutex$(CO) container$(CO) queue$(CO)
OBJECTS = $(OBJECTS) xstring$(CO) smartptr$(CO)

cpputil$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del cpputil$(LBO) $(OBJECTS) 2> nul

event.cpp:     event.h
mutex.cpp:     mutex.h
smartptr.cpp:  smartptr.h mutex.h
simplestr.cpp: simplestr.h
xstring.cpp:   xstring.h
xstring.h:     smartptr.h
queue.cpp:     queue.h
queue.h:       mutex.h smartptr.h
container.cpp: container.h
container.h:   mutex.h smartptr.h

OBJECTS = unittest$(CO)
LIBS    = cpputil$(LBO)

test: unittest.exe
	unittest.exe

unittest.exe: $(OBJECTS) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX)
	$(POSTLINKCMD)
