TOPDIR   = ..\\..
INCDIR   = ../include;$(TOPDIR)/libfaad/include;$(TOPDIR)/include;$(TOPDIR)/utils
TARGET   = libfaad$(LBO)
CFLAGS_USR = -DHAVE_CONFIG_H 
CFLAGS_DBG_USR = -DANALYSIS
#CFLAGS_OPT_USR = 
#AFLAGS_USR = -DOBJ_FORMAT_aout

include $(TOPDIR)\config\makerules

# C files
OBJECTS= bits$(CO) \
	cfft$(CO) \
	decoder$(CO) \
	drc$(CO) \
	drm_dec$(CO) \
	error$(CO) \
	filtbank$(CO) \
	ic_predict$(CO) \
	is$(CO) \
	lt_predict$(CO) \
	mdct$(CO) \
	mp4$(CO) \
	ms$(CO) \
	output$(CO) \
	pns$(CO) \
	ps_dec$(CO) \
	ps_syntax$(CO) \
	pulse$(CO) \
	specrec$(CO) \
	syntax$(CO) \
	tns$(CO) \
	hcr$(CO) \
	huffman$(CO) \
	rvlc$(CO) \
	ssr$(CO) \
	ssr_fb$(CO) \
	ssr_ipqf$(CO) \
	common$(CO) \
	sbr_dct$(CO) \
	sbr_e_nf$(CO) \
	sbr_fbt$(CO) \
	sbr_hfadj$(CO) \
	sbr_hfgen$(CO) \
	sbr_huff$(CO) \
	sbr_qmf$(CO) \
	sbr_syntax$(CO) \
	sbr_tf_grid$(CO) \
	sbr_dec$(CO)

libfaad$(LBO): $(OBJECTS) $(LIBS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libfaad$(LBO) $(OBJECTS) *.e 2> nul
	
# dependencies
bits.c:		common.h structs.h bits.h
common.h:	..\include\neaacdec.h fixed.h
structs.h:	cfft.h sbr_dec.h
sbr_dec.h:	ps_dec.h drm_dec.h
ps_dec.h:	bits.h
bits.h:		analysis.h
drm_dec.h:	bits.h
cfft.c:		common.h structs.h cfft.h cfft_tab.h
common.c:	common.h structs.h syntax.h
syntax.h:	bits.h
decoder.c:	common.h structs.h mp4.h syntax.h error.h output.h filtbank.h drc.h sbr_dec.h sbr_syntax.h ssr.h
mp4.h:		..\include\neaacdec.h
sbr_syntax.h:	bits.h
drc.c:		common.h structs.h syntax.h drc.h
drm_dec.c:	common.h sbr_dec.h drm_dec.h bits.h
error.c:	common.h error.h
filtbank.c:	common.h structs.h filtbank.h syntax.h kbd_win.h sine_win.h mdct.h
hcr.c:		common.h structs.h specrec.h huffman.h
specrec.h:	syntax.h
huffman.c:	common.h structs.h bits.h huffman.h codebook\hcb.h
ic_predict.c:	common.h structs.h syntax.h ic_predict.h pns.h
pns.h:		syntax.h
is.c:		common.h structs.h syntax.h is.h
is.h:		syntax.h
lt_predict.c:	common.h structs.h syntax.h lt_predict.h filtbank.h tns.h
lt_predict.h:	filtbank.h
mdct.c:		common.h structs.h cfft.h mdct.h mdct_tab.h
mp4.c:		common.h structs.h bits.h mp4.h syntax.h
ms.c:		common.h structs.h syntax.h ms.h is.h pns.h
output.c:	common.h structs.h output.h
pns.c:		common.h structs.h pns.h
ps_dec.c:	common.h ps_dec.h ps_tables.h
ps_syntax.c:	common.h bits.h ps_dec.h
pulse.c:	common.h structs.h syntax.h pulse.h
rvlc.c:		common.h structs.h syntax.h bits.h rvlc.h
sbr_dct.c:	common.h sbr_dct.h
sbr_dec.c:	common.h structs.h syntax.h bits.h sbr_syntax.h sbr_qmf.h sbr_hfgen.h sbr_hfadj.h
sbr_e_nf.c:	common.h structs.h sbr_syntax.h sbr_e_nf.h
sbr_fbt.c:	common.h structs.h sbr_syntax.h sbr_fbt.h
sbr_hfadj.c:	common.h structs.h sbr_syntax.h sbr_hfadj.h sbr_noise.h
sbr_hfgen.c:	common.h structs.h sbr_syntax.h sbr_hfgen.h sbr_fbt.h
sbr_huff.c:	common.h structs.h sbr_syntax.h bits.h sbr_huff.h sbr_e_nf.h
sbr_qmf.c:	common.h structs.h sbr_dct.h sbr_qmf.h sbr_qmf_c.h sbr_syntax.h
sbr_syntax.c:	common.h structs.h sbr_syntax.h sbr_huff.h sbr_fbt.h sbr_tf_grid.h sbr_e_nf.h bits.h ps_dec.h drm_dec.h analysis.h
sbr_tf_grid.c:	common.h structs.h sbr_syntax.h sbr_tf_grid.h
specrec.c:	common.h structs.h specrec.h filtbank.h syntax.h iq_table.h ms.h is.h pns.h tns.h drc.h lt_predict.h ic_predict.h ssr.h ssr_fb.h
ssr.c:		common.h structs.h syntax.h filtbank.h ssr.h ssr_fb.h
ssr_fb.c:	common.h structs.h syntax.h filtbank.h mdct.h ssr_fb.h ssr_win.h
ssr_ipqf.c:	common.h structs.h ssr.h ssr_ipqf.h
syntax.c:	common.h structs.h syntax.h specrec.h huffman.h bits.h pulse.h analysis.h drc.h rvlc.h sbr_syntax.h mp4.h
tns.c:		common.h structs.h syntax.h tns.h
