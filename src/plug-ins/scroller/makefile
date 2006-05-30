#
#  scroller.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\utils;..\..\include
LIB      = $(LIB);..\..\utils

all: scroller.dll

DEF      = scroller.def
OBJECTS  = scroller.obj
LIBS     = utilfct.lib

scroller.dll: $(OBJECTS) $(DEF)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(LI) $*.lib $@

clean:
  -@del scroller.dll scroller.lib scroller.map $(OBJECTS) $(RESOURCE) 2> nul
