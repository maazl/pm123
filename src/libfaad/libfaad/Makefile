TOPDIR   = ..\..
INCDIR   = ../include;$(TOPDIR)/libfaad/include;$(TOPDIR)/include;$(TOPDIR)/utils
TARGET   = libfaad$(LBO)
CFLAGS_USR = -DHAVE_CONFIG_H 
#CFLAGS_DBG_USR = -DANALYSIS
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

libfaad$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libfaad$(LBO) $(OBJECTS) *.e 2> nul
	
# dependencies
bits.o:		bits.c structs.h sbr_dec.h drm_dec.h 
sbr_hfadj.o:	sbr_hfadj.c structs.h sbr_dec.h drm_dec.h  sbr_hfadj.h
cfft.o:		cfft.c structs.h sbr_dec.h drm_dec.h  cfft_tab.h
common.o:	common.c structs.h sbr_dec.h drm_dec.h 
decoder.o:	decoder.c structs.h sbr_dec.h drm_dec.h  syntax.h output.h drc.h sbr_syntax.h
sbr_qmf.o:	sbr_qmf.c structs.h sbr_dec.h drm_dec.h  sbr_qmf.h sbr_syntax.h
drc.o:		drc.c structs.h sbr_dec.h drm_dec.h  drc.h
drm_dec.o:	drm_dec.c sbr_dec.h drm_dec.h  bits.h
sbr_hfgen.o:	sbr_hfgen.c structs.h sbr_dec.h drm_dec.h  sbr_hfgen.h
error.o:	error.c error.h
sbr_huff.o:	sbr_huff.c structs.h sbr_dec.h drm_dec.h  bits.h sbr_e_nf.h
filtbank.o:	filtbank.c structs.h sbr_dec.h drm_dec.h  syntax.h sine_win.h
hcr.o:		hcr.c structs.h sbr_dec.h drm_dec.h  huffman.h
huffman.o:	huffman.c structs.h sbr_dec.h drm_dec.h  huffman.h
sbr_syntax.o:	sbr_syntax.c structs.h sbr_dec.h drm_dec.h  sbr_huff.h sbr_tf_grid.h bits.h
sbr_tf_grid.o:	sbr_tf_grid.c structs.h sbr_dec.h drm_dec.h  sbr_tf_grid.h
ic_predict.o:	ic_predict.c structs.h sbr_dec.h drm_dec.h  ic_predict.h
ssr_fb.o:	ssr_fb.c structs.h sbr_dec.h drm_dec.h  filtbank.h ssr_fb.h
is.o:		is.c structs.h sbr_dec.h drm_dec.h  is.h
lt_predict.o:	lt_predict.c structs.h sbr_dec.h drm_dec.h  lt_predict.h tns.h
specrec.o:	specrec.c structs.h sbr_dec.h drm_dec.h  filtbank.h iq_table.h is.h tns.h lt_predict.h ssr.h
ssr_ipqf.o:	ssr_ipqf.c structs.h sbr_dec.h drm_dec.h  ssr_ipqf.h
mdct.o:		mdct.c structs.h sbr_dec.h drm_dec.h  mdct.h
ps_dec.o:	ps_dec.c ps_dec.h 
mp4.o:		mp4.c structs.h sbr_dec.h drm_dec.h  mp4.h
ms.o:		ms.c structs.h sbr_dec.h drm_dec.h  ms.h pns.h
output.o:	output.c structs.h sbr_dec.h drm_dec.h 
tns.o:		tns.c structs.h sbr_dec.h drm_dec.h  tns.h
pns.o:		pns.c structs.h sbr_dec.h drm_dec.h 
ps_syntax.o:	ps_syntax.c bits.h 
sbr_e_nf.o:	sbr_e_nf.c structs.h sbr_dec.h drm_dec.h  sbr_e_nf.h
syntax.o:	syntax.c structs.h sbr_dec.h drm_dec.h  specrec.h bits.h analysis.h rvlc.h mp4.h
pulse.o:	pulse.c structs.h sbr_dec.h drm_dec.h  pulse.h
sbr_fbt.o:	sbr_fbt.c structs.h sbr_dec.h drm_dec.h  sbr_fbt.h
rvlc.o:		rvlc.c structs.h sbr_dec.h drm_dec.h  bits.h
sbr_dct.o:	sbr_dct.c sbr_dct.h
sbr_dec.o:	sbr_dec.c structs.h sbr_dec.h drm_dec.h  bits.h sbr_qmf.h sbr_hfadj.h
ssr.o:		ssr.c structs.h sbr_dec.h drm_dec.h  filtbank.h ssr_fb.h
