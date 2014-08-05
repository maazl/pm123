#
#  mpg123.dll makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = libmpg123$(LBO)

!include $(TOPDIR)\config\makerules

# C files
OBJECTS=compat$(CO) dct64$(CO) dct64_i386$(CO) dither$(CO) equalizer$(CO) feature$(CO)
OBJECTS=$(OBJECTS) format$(CO) frame$(CO) id3$(CO) index$(CO) layer1$(CO) layer2$(CO)
OBJECTS=$(OBJECTS) layer3$(CO) libmpg123$(CO) optimize$(CO) parse$(CO) readers$(CO)
OBJECTS=$(OBJECTS) stringbuf$(CO) synth_real$(CO) tabinit$(CO) tabinit_mmx$(CO)
OBJECTS=$(OBJECTS) testcpu$(CO)
# synth$(CO) 

# Assembler files
OBJECTS=$(OBJECTS) dct36_3dnow$(CO) dct36_3dnowext$(CO) dct64_3dnow$(CO) dct64_3dnowext$(CO)
OBJECTS=$(OBJECTS) dct36_sse$(CO) dct64_sse$(CO) dct64_sse_float$(CO) equalizer_3dnow$(CO) getcpuflags$(CO)
OBJECTS=$(OBJECTS) synth_3dnow$(CO) synth_3dnowext$(CO) synth_i586$(CO)
OBJECTS=$(OBJECTS) synth_sse$(CO) synth_sse_float$(CO)
OBJECTS=$(OBJECTS) synth_stereo_sse_float$(CO)
#OBJECTS=$(OBJECTS) synth_3dnow$(CO) synth_3dnowext$(CO) synth_i586$(CO) synth_i586_dither$(CO)
#OBJECTS=$(OBJECTS) synth_sse$(CO) synth_sse_accurate$(CO) synth_sse_float$(CO) synth_sse_s32$(CO)
#OBJECTS=$(OBJECTS) synth_stereo_sse_accurate$(CO) synth_stereo_sse_float$(CO) synth_stereo_sse_s32$(CO)

libmpg123$(LBO): $(OBJECTS) $(LIBS)
        if exist $@ del $@
        $(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libmpg123$(LBO) $(OBJECTS) *.e 2> nul

compat.c:	..\include\config.h compat.h debug.h
compat.h:	..\include\config.h intsym.h true.h
intsym.h:	mpg123_config.h
debug.h:..\include\config.h
dct64.c:	mpg123lib_intern.h
mpg123lib_intern.h:	..\include\config.h mpg123_config.h intsym.h abi_align.h compat.h mpg123.h true.h optimize.h decode.h parse.h frame.h
parse.h:	frame.h
frame.h:	..\include\config.h mpg123.h optimize.h icy.h reader.h index.h synths.h dither.h
icy.h:		compat.h mpg123.h
reader.h:	..\include\config.h mpg123.h
index.h:	..\include\config.h compat.h
dct64_altivec.c:	mpg123lib_intern.h
dct64_i386.c:	mpg123lib_intern.h
dct64_i486.c:	mpg123lib_intern.h
dither.c:	..\include\config.h compat.h dither.h
equalizer.c:	mpg123lib_intern.h
feature.c:	mpg123lib_intern.h
format.c:	mpg123lib_intern.h debug.h
icy.c:		icy.h
icy2utf8.c:	compat.h
id3.c:		mpg123lib_intern.h id3.h debug.h
id3.h:		frame.h
index.c:	index.h debug.h
layer1.c:	mpg123lib_intern.h getbits.h debug.h
getbits.h:	mpg123lib_intern.h
layer2.c:	mpg123lib_intern.h l2tables.h getbits.h l12_integer_tables.h
layer3.c:	mpg123lib_intern.h newhuffman.h getbits.h debug.h l3_integer_tables.h
lfs_alias.c:	..\include\config.h compat.h abi_align.h
lfs_wrap.c:	mpg123lib_intern.h compat.h debug.h
gapless.h:	debug.h
ntom.c:		mpg123lib_intern.h debug.h
optimize.c:	mpg123lib_intern.h debug.h getcpuflags.h
parse.c:	mpg123lib_intern.h getbits.h mpeghead.h debug.h
stringbuf.c:	mpg123lib_intern.h ..\include\config.h mpg123.h compat.h debug.h
synth.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
synth_8bit.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_8bit.h synth_ntom.h
synth_altivec.c:	mpg123lib_intern.h
synth_i486.c:	mpg123lib_intern.h
synth_real.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
synth_s32.c:	mpg123lib_intern.h sample.h debug.h synth.h synth_mono.h synth_ntom.h
tabinit.c:	mpg123lib_intern.h debug.h
testcpu.c:	getcpuflags.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
abi_align.h:	..\include\config.h
mpg123.h:	mpg123_config.h
getcpuflags_arm.c:	mpg123lib_intern.h getcpuflags.h
readers.c:	mpg123lib_intern.h compat.h debug.h
frame.c:	mpg123lib_intern.h id3.h getcpuflags.h debug.h
libmpg123.c:	mpg123lib_intern.h id3.h icy2utf8.h debug.h gapless.h
