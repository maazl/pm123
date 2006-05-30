#
#  analyzer.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\utils;..\..\include
LIB      = $(LIB);..\..\utils

all: analyzer.dll

DEF      = analyzer.def
OBJECTS  = analyzer.obj
RESOURCE = analyzer.res
LIBS     = mmpm2.lib utilfct.lib

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

analyzer.c:  analyzer.h
analyzer.rc: analyzer.h

clean:
  -@del analyzer.dll analyzer.lib analyzer.map $(OBJECTS) $(RESOURCE) 2> nul
