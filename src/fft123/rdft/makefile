#
#  rdft.lib makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft

!include $(TOPDIR)\..\config\makerules

all: rdft.lib $(MDUMMY)

OBJECTS = buffered.obj buffered2.obj conf.obj dft_r2hc.obj dht_r2hc.obj
OBJECTS = $(OBJECTS) dht_rader.obj direct.obj direct2.obj generic.obj
OBJECTS = $(OBJECTS) hc2hc_buf.obj hc2hc_dif.obj hc2hc_dit.obj hc2hc.obj
OBJECTS = $(OBJECTS) indirect.obj khc2hc_dif.obj khc2hc_dit.obj khc2r.obj
OBJECTS = $(OBJECTS) kr2hc.obj kr2r.obj nop.obj nop2.obj plan.obj
OBJECTS = $(OBJECTS) plan2.obj problem.obj problem2.obj rader_hc2hc.obj
OBJECTS = $(OBJECTS) rank_geq2_rdft2.obj rank_geq2.obj rank0_rdft2.obj
OBJECTS = $(OBJECTS) rank0.obj rdft_dht.obj rdft2_inplace_strides.obj
OBJECTS = $(OBJECTS) rdft2_radix2.obj rdft2_strides.obj
OBJECTS = $(OBJECTS) rdft2_tensor_max_index.obj solve.obj solve2.obj
OBJECTS = $(OBJECTS) vrank_geq1_rdft2.obj vrank_geq1.obj

rdft.lib: $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del rdft.lib $(OBJECTS) 2> nul
