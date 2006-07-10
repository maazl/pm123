#
#  mpg123.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\http123

!include $(TOPDIR)\config\makerules

OBJECTS  = mpg123.obj common.obj decode_i386.obj dct64_MMX.obj decode_2to1.obj
OBJECTS  = $(OBJECTS) decode_4to1.obj layer1.obj layer2.obj layer3.obj
OBJECTS  = $(OBJECTS) dxhead.obj equalizer.obj decode_MMX.obj tabinit_MMX.obj
LIBS     = $(LIBS_TCP) $(TOPDIR)\utils\utilfct.lib $(TOPDIR)\http123\http123.lib
DEF      = mpg123$(LDEF_SUFFX)
RESOURCE = mpg123.res

all: mpg123.dll mpg123.lib $(MDUMMY)

mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

mpg123.lib: $(DEF)
	$(LI) $@ mpg123.dll

layer1.obj:        layer1.c mpg123.h
layer2.obj:        layer2.c mpg123.h
layer3.obj:        layer3.c mpg123.h huffman.h get1bit.h
decode.obj:        decode.c mpg123.h
decode_int.obj:    decode_int.c mpg123.h
decode_2to1.obj:   decode_2to1.c mpg123.h
decode_4to1.obj:   decode_4to1.c mpg123.h
decode_i386.obj:   decode_i386.c mpg123.h
common.obj:        common.c mpg123.h tables.h
mpg123.obj:        mpg123.c mpg123.h
getbits.obj:       getbits.c mpg123.h
tabinit.obj:       tabinit.c mpg123.h
dct64.obj:	   dct64.c mpg123.h
dct64_i386.obj:    dct64_i386.c mpg123.h
equalizer.obj:	   equalizer.c mpg123.h

clean: $(MDUMMY)
	-@del mpg123.lib mpg123.dll $(OBJECTS) $(RESOURCE) 2> nul
