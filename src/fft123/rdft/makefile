#
#  rdft library makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft

!include $(TOPDIR)\..\config\makerules

all: rdft$(LBO) $(MDUMMY)

OBJECTS = buffered$(CO) buffered2$(CO) conf$(CO) dft_r2hc$(CO) dht_r2hc$(CO)
OBJECTS = $(OBJECTS) dht_rader$(CO) direct$(CO) direct2$(CO) generic$(CO)
OBJECTS = $(OBJECTS) hc2hc_buf$(CO) hc2hc_dif$(CO) hc2hc_dit$(CO) hc2hc$(CO)
OBJECTS = $(OBJECTS) indirect$(CO) khc2hc_dif$(CO) khc2hc_dit$(CO) khc2r$(CO)
OBJECTS = $(OBJECTS) kr2hc$(CO) kr2r$(CO) nop$(CO) nop2$(CO) plan$(CO)
OBJECTS = $(OBJECTS) plan2$(CO) problem$(CO) problem2$(CO) rader_hc2hc$(CO)
OBJECTS = $(OBJECTS) rank_geq2_rdft2$(CO) rank_geq2$(CO) rank0_rdft2$(CO)
OBJECTS = $(OBJECTS) rank0$(CO) rdft_dht$(CO) rdft2_inplace_strides$(CO)
OBJECTS = $(OBJECTS) rdft2_radix2$(CO) rdft2_strides$(CO)
OBJECTS = $(OBJECTS) rdft2_tensor_max_index$(CO) solve$(CO) solve2$(CO)
OBJECTS = $(OBJECTS) vrank_geq1_rdft2$(CO) vrank_geq1$(CO)

rdft$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del rdft$(LBO) $(OBJECTS) 2> nul
