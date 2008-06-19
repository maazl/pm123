#
#  mpg123.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123;id3v1;id3v2
TARGET   = mpg123.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = dct64_MMX$(OBJ)  decode_MMX$(OBJ) tabinit_MMX$(OBJ)
OBJECTS  = $(OBJECTS) mpg123$(CO) common$(CO) decode_i386$(CO)
OBJECTS  = $(OBJECTS) layer1$(CO) layer2$(CO) layer3$(CO)
OBJECTS  = $(OBJECTS) dxhead$(CO) equalizer$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) id3v1\id3v1$(LBO) id3v2\id3v2$(LBO) $(TOPDIR)\zlb123\zlb123$(LBI)
SYSLIBS  = $(LIBS_UNI)
DEF      = mpg123$(LDEF_SUFFX)
RESOURCE = mpg123$(RES)
STDLIBS  = $(LIBS_UNI)


mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

id3v1\id3v1$(LBO):
	cd id3v1
	@$(MAKE) $(MFLAGS)
	@cd ..

id3v2\id3v2$(LBO):
	cd id3v2
	@$(MAKE) $(MFLAGS)
	@cd ..

clean: $(MDUMMY)
	-@del mpg123$(LBI) mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul
	cd id3v1
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd id3v2
	@$(MAKE) $(MFLAGS) clean
	@cd ..

depend: $(MDUMMY)
	cd id3v1
	@$(MAKE) $(MFLAGS) depend
	@cd ..
	cd id3v2
	@$(MAKE) $(MFLAGS) depend
	@cd ..
	@$(TOPDIR)\config\makedep.cmd *.c* -r -x -I$(INCDIR)

common.c:	common.h tables.h ..\..\utils\debuglog.h ..\..\utils\strutils.h ..\..\utils\minmax.h
common.h:	dxhead.h ..\..\xio123\xio.h id3v1\id3v1.h id3v2\id3v2.h
decode_i386.c:	common.h
dxhead.c:	dxhead.h
equalizer.c:	common.h
layer1.c:	common.h
layer2.c:	common.h
layer3.c:	..\..\utils\debuglog.h common.h huffman.h
mpg123.c:	mpg123.h ..\..\utils\utilfct.h ..\..\include\output_plug.h ..\..\include\decoder_plug.h ..\..\utils\debuglog.h ..\..\utils\snprintf.h id3v1\id3v1.h id3v2\id3v2.h
mpg123.h:	..\..\include\plugin.h common.h
