#
#  http123.dll makefile for NMAKE and VAC++
#

!include ..\config\makerules

CFLAGS  = $(CFLAGS) $(DLLFLG)
INCLUDE = $(INCLUDE);..\utils
LIB     = $(LIB);..\utils

all: http123.dll

DEF     = http123.def
OBJECTS = sockfile.obj httpget.obj
LIBS    = so32dll.lib tcp32dll.lib utilfct.lib

http123.dll: $(OBJECTS) $(DEF)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $** $(LIBS)
   $(LI) $*.lib $@

clean:
  -@del http123.dll http123.lib http123.map $(OBJECTS) 2> nul

sockfile.obj: httpget.h sockfile.h
httpget.obj:  httpget.h

