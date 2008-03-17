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
LIBS     = $(LIBS_UNI) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) id3v1\id3v1$(LBO) id3v2\id3v2$(LBO) $(TOPDIR)\zlb123\zlb123$(LBI)
DEF      = mpg123$(LDEF_SUFFX)
RESOURCE = mpg123$(RES)
STDLIBS  = $(LIBS_UNI)


mpg123.dll: $(OBJECTS) id3v1\id3v1$(LBO) id3v2\id3v2$(LBO) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
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

tag$(CO):	    tag.c tag.h

layer1$(CO):        layer1.c common.h
layer2$(CO):        layer2.c common.h
layer3$(CO):        layer3.c common.h huffman.h
decode_i386$(CO):   decode_i386.c common.h
common$(CO):        common.c common.h tables.h
mpg123$(CO):        mpg123.c mpg123.h common.h
equalizer$(CO):	    equalizer.c common.h

clean: $(MDUMMY)
	-@del mpg123$(LBI) mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul
	cd id3v1
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd id3v2
	@$(MAKE) $(MFLAGS) clean
	@cd ..

