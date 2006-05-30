#
#  wavplay.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)
INCLUDE  = $(INCLUDE);..\..\include
LIB      = $(LIB)

all: wavplay.dll

DEF      = wavplay.def
OBJECTS  = wavplay.obj wav.obj
LIBS     =

wavplay.dll: $(OBJECTS) $(DEF)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $** $(LIBS)
   $(LI) $*.lib $@

clean:
  -@del wavplay.dll wavplay.lib wavplay.map $(OBJECTS) 2> nul

wavplay.obj: wav.h
wav.obj:     wav.h
