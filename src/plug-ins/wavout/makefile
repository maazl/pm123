#
#  wavout.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\utils;..\..\include
LIB      = $(LIB);..\..\utils

all: wavout.dll

DEF      = wavout.def
OBJECTS  = wavout.obj wav.obj
LIBS     = utilfct.lib
RESOURCE = wavout.res

wavout.dll: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

wav.obj:
   $(CC) $(CFLAGS) /C ..\wavplay\wav.cpp

clean:
  -@del wavout.dll wavout.lib wavout.map $(OBJECTS) $(RESOURCE) 2> nul
