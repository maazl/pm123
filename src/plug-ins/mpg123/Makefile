#
#  mpg123.dll makefile
#

TOPDIR  = ..\..
INCDIR  = $(TOPDIR)/utils;$(TOPDIR)/include;$(TOPDIR)/xio123;$(TOPDIR)/libmpg123;$(TOPDIR)/zlb123;
TARGET  = mpg123.dll

include $(TOPDIR)\config\makerules

OBJECTS = mpg123$(CO) dialog$(CO) genre$(CO)
OBJECTS += id3v1\id3v1$(CO) id3v2\id3v2_strings$(CO) id3v2\id3v2_frame$(CO) id3v2\id3v2$(CO)
LIBS    = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI) $(TOPDIR)\libmpg123\libmpg123$(LBO) \
	$(TOPDIR)\zlb123\zlb123$(LBI) $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
SYSLIBS = $(LIBS_UNI)
DEF     = mpg123$(LDEF_SUFFX)
RESOURCE= mpg123$(RES)
STDLIBS = $(LIBS_UNI)


mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

mpg123$(RES): mpg123.rc mpg123.h

clean: $(MDUMMY)
	-@del mpg123$(LBI) mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul

.o:		.cproject
dialog.o:	dialog.cpp mpg123.h ..\../include\plugin.h ..\../xio123\xio.h ..\../utils\../libmpg123/mpg123.h  ..\../utils\cpp/container/vector.h ..\../utils\charset.h ..\../utils\fileutil.h ..\../utils\debuglog.h ..\../utils\eautils.h
mpg123.o:	mpg123.cpp dialog.h ..\../utils\charset.h ..\../utils\debuglog.h ..\../utils\eautils.h 
id3v1\id3v1.o:	id3v1\id3v1.cpp
genre.o:	genre.c 
id3v2\id3v2.o:	id3v2\id3v2.cpp 
id3v2\id3v2_frame.o:	id3v2\id3v2_frame.cpp
id3v2\id3v2_strings.o:	id3v2\id3v2_strings.cpp
