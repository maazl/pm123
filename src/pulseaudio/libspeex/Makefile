#
#  pulse123.lib
#

#DLL      = no
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\pulseaudio;$(TOPDIR)\fft123\api
TARGET   = all2
CFLAGS_USR = -DHAVE_CONFIG_H -DFLOATING_POINT -DEXPORT= -DUSE_GPL_FFTW3

include $(TOPDIR)\config\makerules

all2: libspeex$(LBO) $(MDUMMY)

# resample$(CO) 
# testdenoise$(CO) testecho$(CO) testenc$(CO) testenc_uwb$(CO) testenc_wb$(CO)
# testjitter$(CO)
OBJECTS = bits$(CO) buffer$(CO) cb_search$(CO) exc_10_16_table$(CO) exc_10_32_table$(CO) \
	exc_20_32_table$(CO) exc_5_256_table$(CO) exc_5_64_table$(CO) exc_8_128_table$(CO) \
	fftwrap$(CO) filterbank$(CO) filters$(CO) gain_table$(CO) gain_table_lbr$(CO) \
	hexc_10_32_table$(CO) hexc_table$(CO) high_lsp_tables$(CO) jitter$(CO) \
	kiss_fft$(CO) kiss_fftr$(CO) lpc$(CO) lsp$(CO) lsp_tables_nb$(CO) ltp$(CO) mdf$(CO) \
	modes$(CO) modes_wb$(CO) nb_celp$(CO) preprocess$(CO) quant_lsp$(CO) \
	sb_celp$(CO) scal$(CO) smallft$(CO) speex$(CO) \
	speex_callbacks$(CO) speex_header$(CO) stereo$(CO) \
	vbr$(CO) vq$(CO) window$(CO)

libspeex$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libspeex$(LBO) $(OBJECTS)    2> nul

testjitter.o:	testjitter.c ..\..\pulseaudio\speex/speex_jitter.h 
vq.o:		vq.c vq.h  arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h vq_arm4.h
bits.o:		bits.c ..\..\pulseaudio\speex/speex_bits.h os_support.h 
buffer.o:	buffer.c os_support.h  ..\..\pulseaudio\speex/speex_buffer.h
window.o:	window.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h
cb_search.o:	cb_search.c cb_search.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h stack_alloc.h math_approx.h  cb_search_sse.h cb_search_bfin.h
modes.o:	modes.c modes.h ..\..\pulseaudio\speex/speex_bits.h  quant_lsp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h sb_celp.h vbr.h
nb_celp.o:	nb_celp.c nb_celp.h ..\..\pulseaudio\speex/speex_bits.h vbr.h  vorbis_psy.h lsp.h quant_lsp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h filters.h vq.h os_support.h
modes_wb.o:	modes_wb.c modes.h ..\..\pulseaudio\speex/speex_bits.h  quant_lsp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h sb_celp.h vbr.h
exc_10_16_table.o:	exc_10_16_table.c
exc_10_32_table.o:	exc_10_32_table.c
preprocess.o:	preprocess.c ..\..\pulseaudio\speex/speex_preprocess.h  arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h filterbank.h os_support.h
exc_20_32_table.o:	exc_20_32_table.c
smallft.o:	smallft.c smallft.h os_support.h 
exc_5_256_table.o:	exc_5_256_table.c
exc_5_64_table.o:	exc_5_64_table.c
speex.o:	speex.c modes.h ..\..\pulseaudio\speex/speex_bits.h 
exc_8_128_table.o:	exc_8_128_table.c
fftwrap.o:	fftwrap.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h smallft.h kiss_fftr.h 
filterbank.o:	filterbank.c filterbank.h  math_approx.h
filters.o:	filters.c filters.h  arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h ltp.h filters_arm4.h
speex_callbacks.o:	speex_callbacks.c ..\..\pulseaudio\speex/speex_callbacks.h  os_support.h
speex_header.o:	speex_header.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h ..\..\pulseaudio\speex/speex.h ..\..\pulseaudio\speex/speex_types.h 
gain_table.o:	gain_table.c
stereo.o:	stereo.c ..\..\pulseaudio\speex/speex_stereo.h ..\..\pulseaudio\speex/speex_bits.h math_approx.h  os_support.h
gain_table_lbr.o:	gain_table_lbr.c
testdenoise.o:	testdenoise.c ..\..\pulseaudio\speex/speex_preprocess.h 
hexc_10_32_table.o:	hexc_10_32_table.c
hexc_table.o:	hexc_table.c
testecho.o:	testecho.c ..\..\pulseaudio\speex/speex_echo.h 
high_lsp_tables.o:	high_lsp_tables.c
jitter.o:	jitter.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h ..\..\pulseaudio\speex/speex_bits.h os_support.h 
kiss_fft.o:	kiss_fft.c _kiss_fft_guts.h math_approx.h  os_support.h
kiss_fftr.o:	kiss_fftr.c os_support.h  _kiss_fft_guts.h math_approx.h
lpc.o:		lpc.c lpc.h 
lsp.o:		lsp.c lsp.h  math_approx.h
testenc.o:	testenc.c ..\..\pulseaudio\speex/speex.h ..\..\pulseaudio\speex/speex_types.h 
lsp_tables_nb.o:	lsp_tables_nb.c
ltp.o:		ltp.c ltp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h filters.h  math_approx.h ltp_sse.h ltp_bfin.h
mdf.o:		mdf.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h fftwrap.h  math_approx.h
quant_lsp.o:	quant_lsp.c quant_lsp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h 
testenc_uwb.o:	testenc_uwb.c ..\..\pulseaudio\speex/speex.h ..\..\pulseaudio\speex/speex_types.h 
resample.o:	resample.c arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h os_support.h  resample_sse.h
testenc_wb.o:	testenc_wb.c ..\..\pulseaudio\speex/speex.h ..\..\pulseaudio\speex/speex_types.h 
sb_celp.o:	sb_celp.c sb_celp.h ..\..\pulseaudio\speex/speex_bits.h  lpc.h stack_alloc.h quant_lsp.h arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h ltp.h os_support.h
scal.o:		scal.c ..\..\pulseaudio\speex/speex_echo.h  arch.h fixed_debug.h fixed_arm5e.h fixed_bfin.h smallft.h
vbr.o:		vbr.c vbr.h 
