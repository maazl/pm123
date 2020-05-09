#
#  dft library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = ..\kernel;$(TOPDIR)\include
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: dft$(LBO) $(MDUMMY)

OBJECTS = buffered$(CO) conf$(CO) ct_dif$(CO) ct_dit$(CO) ct_ditbuf$(CO) ct_ditf$(CO) \
	ct$(CO) direct$(CO) generic$(CO) indirect$(CO) kdft_dif$(CO) \
	kdft_difsq$(CO) kdft_dit$(CO) kdft$(CO) nop$(CO) plan$(CO) \
	problem$(CO) rader_omega$(CO) rader$(CO) rank_geq2$(CO) \
	rank0$(CO) solve$(CO) vrank_geq1$(CO) vrank2_transpose$(CO) \
	vrank3_transpose$(CO) zero$(CO)

dft$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft$(LBO) $(OBJECTS) 2> nul
