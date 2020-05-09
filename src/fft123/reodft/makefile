#
#  reodft library makefile
#

DLL     = yes
TOPDIR  = ..\..
INCDIR  = ..\kernel;..\rdft;$(TOPDIR)\include
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: reodft$(LBO) $(MDUMMY)

OBJECTS = conf$(CO) redft00e_r2hc_pad$(CO) redft00e_r2hc$(CO) reodft010e_r2hc$(CO) \
	reodft11e_r2hc_odd$(CO) reodft11e_r2hc$(CO) \
	reodft11e_radix2$(CO) rodft00e_r2hc_pad$(CO) \
	rodft00e_r2hc$(CO)

reodft$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del reodft$(LBO) $(OBJECTS) 2> nul
