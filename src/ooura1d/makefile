#
#  ooura1d.dll makefile for NMAKE and VAC++
#

!include ..\config\makerules

CFLAGS  = $(CFLAGS) $(DLLFLG) /Ms
INCLUDE = $(INCLUDE)
LIB     = $(LIB)

all: ooura1d.dll

OBJECTS = fft4g.obj
DEF     = ooura1d.def

ooura1d.dll: $(OBJECTS) $(DEF)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $** $(LIBS)
   $(LI) $*.lib $@

clean:
  -@del ooura1d.dll ooura1d.lib ooura1d.map $(OBJECTS) 2> nul
