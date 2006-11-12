#
#  mpg123.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123

!include $(TOPDIR)\config\makerules

OBJECTS  = dct64_MMX$(OBJ)  decode_MMX$(OBJ) tabinit_MMX$(OBJ)
OBJECTS  = $(OBJECTS) mpg123$(CO) common$(CO) decode_i386$(CO) decode_2to1$(CO)
OBJECTS  = $(OBJECTS) decode_4to1$(CO) layer1$(CO) layer2$(CO) layer3$(CO)
OBJECTS  = $(OBJECTS) dxhead$(CO) equalizer$(CO) tag$(CO)
LIBS     = $(LIBS_TCP) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = mpg123$(LDEF_SUFFX)
RESOURCE = mpg123$(RES)
STDLIBS  = $(LIBS_UNI)

all: mpg123.dll mpg123$(LBI) $(MDUMMY)

mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

mpg123$(LBI): $(DEF)
	$(LI) $@ mpg123.dll

layer1$(CO):        layer1.c mpg123.h
layer2$(CO):        layer2.c mpg123.h
layer3$(CO):        layer3.c mpg123.h huffman.h get1bit.h
decode$(CO):        decode.c mpg123.h
decode_int$(CO):    decode_int.c mpg123.h
decode_2to1$(CO):   decode_2to1.c mpg123.h
decode_4to1$(CO):   decode_4to1.c mpg123.h
decode_i386$(CO):   decode_i386.c mpg123.h
common$(CO):        common.c mpg123.h tables.h
mpg123$(CO):        mpg123.c mpg123.h tag.h
getbits$(CO):       getbits.c mpg123.h
tabinit$(CO):       tabinit.c mpg123.h
dct64$(CO):         dct64.c mpg123.h
dct64_i386$(CO):    dct64_i386.c mpg123.h
equalizer$(CO):	    equalizer.c mpg123.h
tag$(CO):	    tag.c tag.h

clean: $(MDUMMY)
	-@del mpg123$(LBI) mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul
