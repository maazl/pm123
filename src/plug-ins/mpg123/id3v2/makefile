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
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

id3v2.c:	 ..\..\..\include\config.h ..\..\..\utils\charset.h ..\..\..\utils\debuglog.h ..\..\..\utils\minmax.h id3v2.h id3v2_header.h
id3v2.h:	 ..\..\..\xio123\xio.h
id3v2_frame.c:	 ..\..\..\include\config.h ..\..\..\zlb123\zlib.h ..\..\..\utils\debuglog.h id3v2.h id3v2_header.h
id3v2_strings.c: ..\..\..\include\config.h ..\..\..\utils\strutils.h ..\..\..\utils\charset.h id3v2.h id3v2_header.h ../id3v1/genre.h
