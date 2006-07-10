#
#  ciscgi.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..

!include $(TOPDIR)\config\makerules

OBJECTS = cgifldld.obj cgiupper.obj urldcode.obj urlncode.obj urlvalue.obj

all: ciscgi.lib $(MDUMMY)

ciscgi.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

cgifldld.obj: cgifldld.c cisrcl.h
cgiupper.obj: cgiupper.c cisrcl.h
urldcode.obj: urldcode.c cisrcl.h
urlncode.obj: urlncode.c cisrcl.h
urlvalue.obj: urlvalue.c cisrcl.h

clean: $(MDUMMY)
	-@del ciscgi.lib $(OBJECTS) 2> nul
