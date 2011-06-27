#
#  mpg123.dll makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = all2

!include $(TOPDIR)\config\makerules

# C files
OBJECTS=compat$(CO) \
dct64$(CO) \
dct64_i386$(CO) \
#dct64_i486$(CO) \
dither$(CO) \
equalizer$(CO) \
feature$(CO) \
format$(CO) \
frame$(CO) \
#icy$(CO) \
#icy2utf8$(CO) \
id3$(CO) \
index$(CO) \
layer1$(CO) \
layer2$(CO) \
layer3$(CO) \
#lfs_alias$(CO) \
#lfs_wrap$(CO) \
libmpg123$(CO) \
#ntom$(CO) \
optimize$(CO) \
parse$(CO) \
readers$(CO) \
stringbuf$(CO) \
synth$(CO) \
#synth_8bit$(CO) \
#synth_i486$(CO) \
synth_real$(CO) \
#synth_s32$(CO) \
tabinit$(CO) \
tabinit_mmx$(CO) \
testcpu$(CO)

# Assembler files
OBJECTS=$(OBJECTS) \
dct36_3dnow$(CO) \
dct36_3dnowext$(CO) \
dct64_3dnow$(CO) \
dct64_3dnowext$(CO) \
#dct64_mmx$(CO) \
dct64_sse$(CO) \
dct64_sse_float$(CO) \
equalizer_3dnow$(CO) \
getcpuflags$(CO) \
synth_3dnow$(CO) \
synth_3dnowext$(CO) \
synth_i586$(CO) \
synth_i586_dither$(CO) \
#synth_mmx$(CO) \
synth_sse$(CO) \
synth_sse_accurate$(CO) \
synth_sse_float$(CO) \
synth_sse_s32$(CO) \
synth_stereo_sse_accurate$(CO) \
synth_stereo_sse_float$(CO) \
synth_stereo_sse_s32$(CO) \
#tabinit_mmx$(CO)

all2: libmpg123$(LBO) $(MDUMMY)

libmpg123$(LBO): $(OBJECTS) $(LIBS)
        if exist $@ del $@
        $(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libmpg123$(LBO) $(OBJECTS) *.e 2> nul

compat.c:	config.h compat.h ..\include\sys\io.h debug.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
compat.h:	config.h intsym.h
debug.h:	config.h
dct64.c:	mpg123lib_intern.h
mpg123lib_intern.h:	config.h intsym.h compat.h mpg123.h true.h optimize.h decode.h parse.h frame.h
parse.h:	frame.h
frame.h:	config.h mpg123.h optimize.h icy.h reader.h index.h synths.h dither.h
icy.h:		compat.h mpg123.h
reader.h:	config.h mpg123.h
index.h:	config.h compat.h
dct64_altivec.c:	mpg123lib_intern.h
dct64_i386.c:	mpg123lib_intern.h
dct64_i486.c:	mpg123lib_intern.h
dither.c:	compat.h dither.h
equalizer.c:	mpg123lib_intern.h
feature.c:	mpg123lib_intern.h
format.c:	mpg123lib_intern.h debug.h
frame.c:	mpg123lib_intern.h getcpuflags.h id3.h debug.h
id3.h:		frame.h
icy.c:		icy.h
icy2utf8.c:	compat.h
id3.c:		mpg123lib_intern.h id3.h debug.h
index.c:	index.h debug.h
layer1.c:	mpg123lib_intern.h getbits.h
getbits.h:	mpg123lib_intern.h
layer2.c:	mpg123lib_intern.h l2tables.h getbits.h l12_integer_tables.h
layer3.c:	mpg123lib_intern.h huffman.h getbits.h debug.h l3_integer_tables.h
lfs_alias.c:	compat.h mpg123.h mpg123lib_intern.h
lfs_wrap.c:	mpg123lib_intern.h compat.h debug.h
libmpg123.c:	mpg123lib_intern.h icy2utf8.h id3.h debug.h
ntom.c:		mpg123lib_intern.h debug.h
optimize.c:	mpg123lib_intern.h debug.h getcpuflags.h
parse.c:	mpg123lib_intern.h id3.h getbits.h debug.h
readers.c:	mpg123lib_intern.h ..\include\sys\io.h compat.h debug.h
stringbuf.c:	mpg123lib_intern.h mpg123.h compat.h debug.h
synth.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
synth_8bit.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_8bit.h synth_ntom.h
synth_altivec.c:	mpg123lib_intern.h
synth_i486.c:	mpg123lib_intern.h
synth_real.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
synth_s32.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
tabinit.c:	mpg123lib_intern.h debug.h
testcpu.c:	getcpuflags.h
config.h:	..\include\config.h