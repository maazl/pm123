#
#  reodft library makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft

!include $(TOPDIR)\..\config\makerules

all: reodft$(LBO) $(MDUMMY)

OBJECTS = conf$(CO) redft00e_r2hc_pad$(CO) redft00e_r2hc$(CO) reodft010e_r2hc$(CO)
OBJECTS = $(OBJECTS) reodft11e_r2hc_odd$(CO) reodft11e_r2hc$(CO)
OBJECTS = $(OBJECTS) reodft11e_radix2$(CO) rodft00e_r2hc_pad$(CO)
OBJECTS = $(OBJECTS) rodft00e_r2hc$(CO)

reodft$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del reodft$(LBO) $(OBJECTS) 2> nul
