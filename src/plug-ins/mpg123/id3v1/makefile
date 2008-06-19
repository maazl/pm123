#
#  id3v1 library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR   = $(TOPDIR)\xio123;$(TOPDIR)\utils;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

OBJECTS = id3v1$(CO)

all: id3v1$(LBO) $(MDUMMY)

id3v1$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean:  $(MDUMMY)
	-@del id3v1$(LBO) $(OBJECTS) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

id3v1.c:	id3v1.h ..\..\..\utils\minmax.h ..\..\..\utils\snprintf.h ..\..\..\utils\charset.h
id3v1.h:	..\..\..\xio123\xio.h
