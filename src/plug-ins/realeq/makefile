#
#  realeq.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\fft123\api;..\..\utils;..\..\include
LIB      = $(LIB);..\..\fft123;..\..\utils

all: realeq.dll

DEF      = realeq.def
OBJECTS  = realeq.obj mmxfir.obj
RESOURCE = realeq.res
LIBS     = fft123.lib utilfct.lib

realeq.dll: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

clean:
  -@del realeq.dll realeq.lib realeq.map mmxfir.lst $(OBJECTS) $(RESOURCE) 2> nul

realeq.obj: realeq.h
realeq.res: realeq.h
