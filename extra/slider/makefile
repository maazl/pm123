#
#  slider.exe makefile for NMAKE and VAC++
#

!include ..\..\src\config\makerules

all: slider.exe

LNKFLG   = $(LNKFLG) -B"/PM:PM"
INCLUDE  = $(INCLUDE);..\..\src\utils
LIB      = $(LIB);..\..\src\utils

OBJECTS  = slider.obj $(DEF)
RESOURCE = slider.res

slider.exe: $(OBJECTS) $(RESOURCE)
   $(CC) $(LNKFLG) -Fm$* -Fe$@ $(OBJECTS) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul

clean:
  -@del slider.exe slider.map $(OBJECTS) $(RESOURCE)
