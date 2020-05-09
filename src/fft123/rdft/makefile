#
#  rdft library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = ..\kernel;..\dft;$(TOPDIR)\include
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: rdft$(LBO) $(MDUMMY)

OBJECTS = buffered$(CO) buffered2$(CO) conf$(CO) dft_r2hc$(CO) dht_r2hc$(CO) \
	dht_rader$(CO) direct$(CO) direct2$(CO) generic$(CO) \
	hc2hc_buf$(CO) hc2hc_dif$(CO) hc2hc_dit$(CO) hc2hc$(CO) \
	indirect$(CO) khc2hc_dif$(CO) khc2hc_dit$(CO) khc2r$(CO) \
	kr2hc$(CO) kr2r$(CO) nop$(CO) nop2$(CO) plan$(CO) \
	plan2$(CO) problem$(CO) problem2$(CO) rader_hc2hc$(CO) \
	rank_geq2_rdft2$(CO) rank_geq2$(CO) rank0_rdft2$(CO) \
	rank0$(CO) rdft_dht$(CO) rdft2_inplace_strides$(CO) \
	rdft2_radix2$(CO) rdft2_strides$(CO) \
	rdft2_tensor_max_index$(CO) solve$(CO) solve2$(CO) \
	vrank_geq1_rdft2$(CO) vrank_geq1$(CO)

rdft$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del rdft$(LBO) $(OBJECTS) 2> nul
