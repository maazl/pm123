TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = libmpg123$(LBO)

include $(TOPDIR)\config\makerules

# C files
OBJECTS=compat$(CO) dct64$(CO) dct64_i386$(CO) dither$(CO) equalizer$(CO) feature$(CO) \
	format$(CO) frame$(CO) id3$(CO) index$(CO) layer1$(CO) layer2$(CO) \
	layer3$(CO) libmpg123$(CO) optimize$(CO) parse$(CO) readers$(CO) \
	stringbuf$(CO) synth_real$(CO) tabinit$(CO) tabinit_mmx$(CO) \
	testcpu$(CO)
# synth$(CO) 

# Assembler files
OBJECTS+=dct36_3dnow$(CO) dct36_3dnowext$(CO) dct64_3dnow$(CO) dct64_3dnowext$(CO) \
	dct36_sse$(CO) dct64_sse$(CO) dct64_sse_float$(CO) equalizer_3dnow$(CO) getcpuflags$(CO) \
	synth_3dnow$(CO) synth_3dnowext$(CO) synth_i586$(CO) synth_i586_dither$(CO) \
	synth_sse$(CO) synth_sse_accurate$(CO) synth_sse_float$(CO) synth_sse_s32$(CO) \
	synth_stereo_sse_accurate$(CO) synth_stereo_sse_float$(CO) synth_stereo_sse_s32$(CO)

libmpg123$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libmpg123$(LBO) $(OBJECTS) *.e 2>nul

index.o:	index.c debug.h 
compat.o:	compat.c compat.h intsym.h 
layer1.o:	layer1.c getbits.h 
layer2.o:	layer2.c l2tables.h l12_integer_tables.h
layer3.o:	layer3.c newhuffman.h debug.h 
dither.o:	dither.c compat.h intsym.h 
lfs_alias.o:	lfs_alias.c compat.h intsym.h 
lfs_wrap.o:	lfs_wrap.c compat.h intsym.h 
libmpg123.o:	libmpg123.c id3.h  debug.h
format.o:	format.c debug.h 
dct64.o:	dct64.c 
frame.o:	frame.c id3.h  debug.h
dct64_altivec.o:	dct64_altivec.c 
dct64_i386.o:	dct64_i386.c 
ntom.o:		ntom.c debug.h 
dct64_i486.o:	dct64_i486.c 
optimize.o:	optimize.c debug.h 
parse.o:	parse.c getbits.h  debug.h
getcpuflags_arm.o:	getcpuflags_arm.c getcpuflags.h
icy.o:		icy.c 
equalizer.o:	equalizer.c 
readers.o:	readers.c compat.h intsym.h 
icy2utf8.o:	icy2utf8.c 
id3.o:		id3.c id3.h 
feature.o:	feature.c 
stringbuf.o:	stringbuf.c ..\include\config.h  compat.h intsym.h
synth.o:	synth.c sample.h synth.h synth_ntom.h
synth_8bit.o:	synth_8bit.c sample.h synth.h synth_8bit.h 
synth_altivec.o:	synth_altivec.c 
synth_i486.o:	synth_i486.c 
tabinit.o:	tabinit.c debug.h 
testcpu.o:	testcpu.c 
synth_real.o:	synth_real.c sample.h synth.h synth_ntom.h
synth_s32.o:	synth_s32.c sample.h synth.h synth_ntom.h
