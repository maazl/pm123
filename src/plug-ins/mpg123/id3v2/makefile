#
#  id3v2 library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR   = $(TOPDIR)\xio123;$(TOPDIR)\zlb123;$(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS = id3v2_strings$(CO) id3v2_frame$(CO) id3v2$(CO)

all: id3v2$(LBO) $(MDUMMY)

id3v2$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del id3v2$(LBO) $(OBJECTS) 2> nul
