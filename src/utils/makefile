#
#  utilfct.lib makefile for NMAKE and VAC++
#

!include ..\config\makerules

INCLUDE = $(INCLUDE);
LIB     = $(LIB);

all: utilfct.lib $(MKSYMB)

OBJECTS = utilfct.obj bufstream.obj fileutil.obj url.obj filefind.obj
OBJECTS = $(OBJECTS) errorstr.obj rel2abs.obj abs2rel.obj inimacro.obj
OBJECTS = $(OBJECTS) charset.obj wildcards.obj queue.obj

utilfct.lib: $(OBJECTS)
   if exist $@ del $@
   $(LB) $@ $(OBJECTS) $(LE)

clean: $(MKSYMB)
  -@del utilfct.lib oddfct.lst $(OBJECTS) $(RESOURCE) 2> nul
