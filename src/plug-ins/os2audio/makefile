#
#  os2audio.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\utils;..\..\include
LIB      = $(LIB);..\..\utils

all: os2audio.dll

DEF      = os2audio.def
OBJECTS  = os2audio.obj
LIBS     = mmpm2.lib utilfct.lib
RESOURCE = os2audio.res

os2audio.dll: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

clean:
  -@del os2audio.dll os2audio.map os2audio.lib $(OBJECTS) $(RESOURCE) 2> nul
