#
#  slider.exe makefile for NMAKE and VAC++
#

!include ..\..\src\config\makerules

all: querysong.exe

LNKFLG   = $(LNKFLG)

OBJECTS  = querysong.obj $(DEF)

querysong.exe: $(OBJECTS) $(RESOURCE)
   $(CC) $(LNKFLG) -Fm$* -Fe$@ $(OBJECTS) $(LIBS)

clean:
  -@del querysong.exe querysong.map $(OBJECTS)
