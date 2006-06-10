#
#  utilfct.lib makefile for NMAKE and VAC++
#

!include ..\config\makerules

INCLUDE = $(INCLUDE);
LIB     = $(LIB);

all: utilfct.lib

OBJECTS = utilfct.obj  bufstream.obj fileutil.obj  url.obj \
          filefind.obj errorstr.obj  rel2abs.obj   abs2rel.obj \
          inimacro.obj charset.obj   wildcards.obj queue.obj

utilfct.lib: $(OBJECTS)
   if exist $@ del $@
   $(LB) $@ $(OBJECTS);

clean:
  -@del utilfct.lib oddfct.lst $(OBJECTS) $(RESOURCE) 2> nul
