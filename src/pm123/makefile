#
#  pm123.exe makefile for NMAKE and VAC++
#

!include ..\config\makerules

LNKFLG   = $(LNKFLG) -B"/PM:PM /STACK:1048576"
INCLUDE  = $(INCLUDE);..\include;..\utils;..\gbm123;..\http123;..\fft123\api
LIB      = $(LIB);..\utils;..\gbm123;..\http123;..\fft123

all: pm123.exe

DEF      = pm123.def
OBJECTS  = button95.obj pm123.obj 123_msg.obj 123_util.obj tag.obj \
           pfreq.obj specana.obj plugman.obj bookmark.obj iniman.obj \
           playlist.obj properties.obj skin.obj \
           docking.obj
RESOURCE = pm123.res
LIBS     = gbm123.lib http123.lib utilfct.lib fft123.lib

pm123.exe: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) -Fm$* -Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   execmode -sp pm123.exe

pm123.c:          pm123.h    \
                  plugman.h  \
                  bookmark.h \
                  button95.h \
                  pfreq.h    \
                  genre.h    \
                  docking.h  \
                  copyright.h

docking.c:        docking.h

iniman.c:         pm123.h    \
                  plugman.h  \
                  iniman.h

playlist.c:       pm123.h    \
                  playlist.h

properties.c:     pm123.h    \
                  plugman.h  \
                  copyright.h\
                  properties.h

bookmark.c:       pm123.h    \
                  bookmark.h

skin.c:           pm123.h

pm123.rc:         pm123.h    \
                  bookmark.h
pm123.h:          playlist.h \
                  skin.h     \
                  properties.h
playlist.h:       tag.h

clean:
  -@del pm123.exe pm123.map $(OBJECTS) $(RESOURCE) 2> nul
