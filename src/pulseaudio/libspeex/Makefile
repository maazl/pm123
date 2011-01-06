#
#  pulse123.lib
#

#DLL      = no
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\pulseaudio;$(TOPDIR)\fft123\api
TARGET   = all2
CFLAGS_USR = -DHAVE_CONFIG_H -DFLOATING_POINT -DEXPORT= -DUSE_GPL_FFTW3

!include $(TOPDIR)\config\makerules

all2: libspeex$(LBO) $(MDUMMY)

# resample$(CO) 
# testdenoise$(CO) testecho$(CO) testenc$(CO) testenc_uwb$(CO) testenc_wb$(CO)
# testjitter$(CO)
OBJECTS = bits$(CO) buffer$(CO) cb_search$(CO) exc_10_16_table$(CO) exc_10_32_table$(CO)
OBJECTS = $(OBJECTS) exc_20_32_table$(CO) exc_5_256_table$(CO) exc_5_64_table$(CO) exc_8_128_table$(CO)
OBJECTS = $(OBJECTS) fftwrap$(CO) filterbank$(CO) filters$(CO) gain_table$(CO) gain_table_lbr$(CO)
OBJECTS = $(OBJECTS) hexc_10_32_table$(CO) hexc_table$(CO) high_lsp_tables$(CO) jitter$(CO)
OBJECTS = $(OBJECTS) kiss_fft$(CO) kiss_fftr$(CO) lpc$(CO) lsp$(CO) lsp_tables_nb$(CO) ltp$(CO) mdf$(CO)
OBJECTS = $(OBJECTS) modes$(CO) modes_wb$(CO) nb_celp$(CO) preprocess$(CO) quant_lsp$(CO)
OBJECTS = $(OBJECTS) sb_celp$(CO) scal$(CO) smallft$(CO) speex$(CO)
OBJECTS = $(OBJECTS) speex_callbacks$(CO) speex_header$(CO) stereo$(CO)
OBJECTS = $(OBJECTS) vbr$(CO) vq$(CO) window$(CO)

libspeex$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libspeex$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul
bits.c:		..\..\include\config.h ..\speex\speex_bits.h arch.h os_support.h
arch.h:		..\speex\speex_types.h fixed_debug.h fixed_generic.h fixed_arm5e.h fixed_arm4.h fixed_bfin.h
buffer.c:	..\..\include\config.h os_support.h arch.h ..\speex\speex_buffer.h
..\speex\speex_buffer.h:	..\speex\speex_types.h
cb_search.c:	..\..\include\config.h cb_search.h filters.h stack_alloc.h vq.h math_approx.h os_support.h cb_search_sse.h cb_search_arm4.h cb_search_bfin.h
cb_search.h:	..\speex\speex_bits.h arch.h
filters.h:	arch.h
vq.h:		arch.h
math_approx.h:	arch.h
fftwrap.c:	..\..\include\config.h arch.h os_support.h smallft.h ..\..\fft123\api\fftw3.h kiss_fftr.h kiss_fft.h
kiss_fftr.h:	kiss_fft.h
kiss_fft.h:	arch.h
filterbank.c:	..\..\include\config.h filterbank.h arch.h math_approx.h os_support.h
filterbank.h:	arch.h
filters.c:	..\..\include\config.h filters.h stack_alloc.h arch.h math_approx.h ltp.h filters_sse.h filters_arm4.h filters_bfin.h
ltp.h:		..\speex\speex_bits.h arch.h
jitter.c:	..\..\include\config.h arch.h ..\speex\speex.h ..\speex\speex_bits.h ..\speex\speex_jitter.h os_support.h
..\speex\speex.h:	..\speex\speex_bits.h ..\speex\speex_types.h
..\speex\speex_jitter.h:	..\speex\speex_types.h
kiss_fft.c:	..\..\include\config.h _kiss_fft_guts.h arch.h os_support.h
_kiss_fft_guts.h:	kiss_fft.h math_approx.h arch.h
kiss_fftr.c:	..\..\include\config.h os_support.h kiss_fftr.h _kiss_fft_guts.h
lpc.c:		..\..\include\config.h lpc.h lpc_bfin.h
lpc.h:		arch.h
lsp.c:		..\..\include\config.h lsp.h stack_alloc.h math_approx.h lsp_bfin.h
lsp.h:		arch.h
ltp.c:		..\..\include\config.h ltp.h stack_alloc.h filters.h ..\speex\speex_bits.h math_approx.h os_support.h ltp_sse.h ltp_arm4.h ltp_bfin.h
mdf.c:		..\..\include\config.h arch.h ..\speex\speex_echo.h fftwrap.h pseudofloat.h math_approx.h os_support.h
fftwrap.h:	arch.h
pseudofloat.h:	arch.h os_support.h math_approx.h
modes.c:	..\..\include\config.h modes.h ltp.h quant_lsp.h cb_search.h sb_celp.h nb_celp.h vbr.h
modes.h:	..\speex\speex.h ..\speex\speex_bits.h arch.h
quant_lsp.h:	..\speex\speex_bits.h arch.h
sb_celp.h:	modes.h ..\speex\speex_bits.h nb_celp.h
nb_celp.h:	modes.h ..\speex\speex_bits.h ..\speex\speex_callbacks.h vbr.h filters.h vorbis_psy.h
vbr.h:		arch.h
vorbis_psy.h:	smallft.h
modes_wb.c:	..\..\include\config.h modes.h ltp.h quant_lsp.h cb_search.h sb_celp.h nb_celp.h vbr.h os_support.h
nb_celp.c:	..\..\include\config.h nb_celp.h lpc.h lsp.h ltp.h quant_lsp.h cb_search.h filters.h stack_alloc.h vq.h ..\speex\speex_bits.h vbr.h math_approx.h os_support.h ..\speex\speex_callbacks.h vorbis_psy.h
preprocess.c:	..\..\include\config.h ..\speex\speex_preprocess.h ..\speex\speex_echo.h arch.h fftwrap.h filterbank.h math_approx.h os_support.h
quant_lsp.c:	..\..\include\config.h quant_lsp.h os_support.h arch.h quant_lsp_bfin.h
resample.c:	..\..\include\config.h arch.h ..\speex\speex_resampler.h os_support.h stack_alloc.h resample_sse.h
sb_celp.c:	..\..\include\config.h sb_celp.h filters.h lpc.h lsp.h stack_alloc.h cb_search.h quant_lsp.h vq.h ltp.h math_approx.h os_support.h
scal.c:		..\..\include\config.h ..\speex\speex_echo.h vorbis_psy.h arch.h os_support.h smallft.h
smallft.c:	..\..\include\config.h smallft.h arch.h os_support.h
speex.c:	..\..\include\config.h modes.h os_support.h
speex_callbacks.c:	..\..\include\config.h ..\speex\speex_callbacks.h arch.h os_support.h
speex_header.c:	..\..\include\config.h arch.h ..\speex\speex_header.h ..\speex\speex.h os_support.h
..\speex\speex_header.h:	..\speex\speex_types.h
stereo.c:	..\..\include\config.h ..\speex\speex_stereo.h ..\speex\speex_callbacks.h math_approx.h vq.h os_support.h
..\speex\speex_stereo.h:	..\speex\speex_types.h ..\speex\speex_bits.h
testdenoise.c:	..\..\include\config.h ..\speex\speex_preprocess.h
..\speex\speex_preprocess.h:	..\speex\speex_types.h
testenc.c:	..\..\include\config.h ..\speex\speex.h ..\speex\speex_callbacks.h
testenc_uwb.c:	..\..\include\config.h ..\speex\speex.h ..\speex\speex_callbacks.h
testenc_wb.c:	..\..\include\config.h ..\speex\speex.h ..\speex\speex_callbacks.h
testjitter.c:	..\..\include\config.h ..\speex\speex_jitter.h
vbr.c:		..\..\include\config.h vbr.h
vq.c:		..\..\include\config.h vq.h stack_alloc.h arch.h vq_sse.h vq_arm4.h vq_bfin.h
window.c:	..\..\include\config.h arch.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
testecho.c:	..\..\include\config.h ..\speex\speex_echo.h ..\speex\speex_preprocess.h
os_support.h:	..\..\include\config.h
..\speex\speex_echo.h:	..\speex\speex_types.h
..\speex\speex_resampler.h:	..\speex\speex_types.h
..\speex\speex_callbacks.h:	..\speex\speex.h
..\..\fft123\api\fftw3.h:	..\..\include\config.h
