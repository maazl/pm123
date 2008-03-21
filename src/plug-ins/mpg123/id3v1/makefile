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

id3v1$(CO): id3v1.c id3v1.h

clean: $(MDUMMY)
	-@del id3v1$(LBO) $(OBJECTS) 2> nul
id3v1.c:	id3v1.h ..\..\..\utils\minmax.h ..\..\..\utils\snprintf.h ..\..\..\utils\charset.h genre.h
id3v1.h:	..\..\..\xio123\xio.h
