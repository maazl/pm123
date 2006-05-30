#
#  ciscgi.lib makefile for NMAKE and VAC++
#

!include ..\..\..\config\makerules

CFLAGS   = $(CFLAGS) $(DLLFLG)

all: ciscgi.lib

OBJECTS  = cgifldld.obj cgiupper.obj urldcode.obj urlncode.obj urlvalue.obj

ciscgi.lib: $(OBJECTS)
   if exist $@ del $@
   $(LB) $@ $(OBJECTS);

clean:
  -@del ciscgi.lib ciscgi.BAK $(OBJECTS) 2> nul

cgifldld.obj: cisrcl.h
cgiupper.obj: cisrcl.h
urldcode.obj: cisrcl.h
urlncode.obj: cisrcl.h
urlvalue.obj: cisrcl.h
