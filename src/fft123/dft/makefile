#
#  dft library makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel

!include $(TOPDIR)\..\config\makerules

all: dft$(LBO) $(MDUMMY)

OBJECTS = buffered$(CO) conf$(CO) ct_dif$(CO) ct_dit$(CO) ct_ditbuf$(CO) ct_ditf$(CO)
OBJECTS = $(OBJECTS) ct$(CO) direct$(CO) generic$(CO) indirect$(CO) kdft_dif$(CO)
OBJECTS = $(OBJECTS) kdft_difsq$(CO) kdft_dit$(CO) kdft$(CO) nop$(CO) plan$(CO)
OBJECTS = $(OBJECTS) problem$(CO) rader_omega$(CO) rader$(CO) rank_geq2$(CO)
OBJECTS = $(OBJECTS) rank0$(CO) solve$(CO) vrank_geq1$(CO) vrank2_transpose$(CO)
OBJECTS = $(OBJECTS) vrank3_transpose$(CO) zero$(CO)

dft$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft$(LBO) $(OBJECTS) 2> nul
