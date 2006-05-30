#
#  cddaplay.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);.\ciscgi;..\..\utils;..\..\include
LIB      = $(LIB);..\..\utils

all: cddaplay.dll

DEF      = cdda.def
OBJECTS  = cdda.obj readcd.obj tcpipsock.obj http.obj cddb.obj ciscgi\ciscgi.lib
LIBS     = so32dll.lib tcp32dll.lib utilfct.lib
RESOURCE = cdda.res

cddaplay.dll: $(OBJECTS) $(DEF) $(RESOURCE) 
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

ciscgi\ciscgi.lib:
   cd ciscgi 
   @$(MAKE) all /nologo
   cd ..

clean:
   cd ciscgi 
   @$(MAKE) clean /nologo /i
   @cd ..
  -@del cddaplay.dll cddaplay.lib cddaplay.map $(OBJECTS) $(RESOURCE) 2> nul

cdda.obj:      readcd.h tcpipsock.h http.h cddb.h
readcd.obj:    readcd.h
tcpipsock.obj: tcpipsock.h
http.obj:      tcpipsock.h http.h
cddb.obj:      readcd.h tcpipsock.h http.h cddb.h
