#
#  mpg123.dll makefile for NMAKE and VAC++
#

!include ..\..\config\makerules

INCLUDE  = $(INCLUDE);..\..\http123;..\..\utils;..\..\include
LIB      = $(LIB);..\..\http123;..\..\utils
CFLAGS   = $(CFLAGS) $(DLLFLG) /DREAL_IS_FLOAT /DPENTIUM_OPT

DEF      = mpg123.def

all: mpg123.dll

OBJECTS  = mpg123.obj common.obj decode_i386.obj dct64_MMX.obj \
           decode_2to1.obj decode_4to1.obj \
           layer1.obj layer2.obj layer3.obj dxhead.obj \
           equalizer.obj decode_i586.obj decode_MMX.obj tabinit_MMX.obj

LIBS     = http123.lib utilfct.lib
RESOURCE = mpg123.res

mpg123.dll: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)
   $(RC) $(RESFLG) $(RESOURCE) $@ > nul
   $(LI) $*.lib $@

clean:
  -@del mpg123.dll mpg123.lib mpg123.map $(OBJECTS) $(RESOURCE) 2> nul

layer1.obj:        mpg123.h
layer2.obj:        mpg123.h
layer3.obj:        mpg123.h huffman.h get1bit.h
decode.obj:        mpg123.h
decode_int.obj:    mpg123.h
decode_2to1.obj:   mpg123.h
decode_4to1.obj:   mpg123.h
decode_i386.obj:   mpg123.h
common.obj:        mpg123.h tables.h
mpg123.obj:        mpg123.h version.h
getbits.obj:       mpg123.h
getbits_.obj:      mpg123.h
getbits_bsdos.obj: mpg123.h
tabinit.obj:       mpg123.h
getlopt.obj:       getlopt.h
dct64.obj:	   mpg123.h
dct64_i386.obj:    mpg123.h
equalizer.obj:	   mpg123.h
