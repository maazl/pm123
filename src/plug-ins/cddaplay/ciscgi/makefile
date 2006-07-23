#
#  ciscgi library makefile
#

DLL     = yes
TOPDIR  = ..\..\..

!include $(TOPDIR)\config\makerules

OBJECTS = cgifldld$(CO) cgiupper$(CO) urldcode$(CO) urlncode$(CO) urlvalue$(CO)

all: ciscgi$(LBO) $(MDUMMY)

ciscgi$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

cgifldld$(CO): cgifldld.c cisrcl.h
cgiupper$(CO): cgiupper.c cisrcl.h
urldcode$(CO): urldcode.c cisrcl.h
urlncode$(CO): urlncode.c cisrcl.h
urlvalue$(CO): urlvalue.c cisrcl.h

clean: $(MDUMMY)
	-@del ciscgi$(LBO) $(OBJECTS) 2> nul
