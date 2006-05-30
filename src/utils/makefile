#
#  utilfct.lib makefile for NMAKE and VAC++
#

!include ..\config\makerules

INCLUDE = $(INCLUDE);
LIB     = $(LIB);

all: utilfct.lib

OBJECTS = utilfct.obj oddfct.obj pmutil.obj tree.obj bufstream.obj \
          find.obj nerrno_str.obj list.obj rel2abs.obj abs2rel.obj \
          charset.obj wildcards.obj

utilfct.lib: $(OBJECTS)
   if exist $@ del $@
   $(LB) $@ $(OBJECTS);

clean:
  -@del utilfct.lib oddfct.lst $(OBJECTS) $(RESOURCE) 2> nul
