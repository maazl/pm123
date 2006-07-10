#
#  r2hc.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft;$(TOPDIR)\rdft\codelets

!include $(TOPDIR)\..\config\makerules

all: r2hc.lib $(MDUMMY)

OBJECTS = hf2_16.obj hf2_32.obj hf2_4.obj hf2_64.obj hf2_8.obj hf_10.obj hf_12.obj
OBJECTS = $(OBJECTS) hf_15.obj hf_16.obj hf_2.obj hf_3.obj hf_32.obj hf_4.obj
OBJECTS = $(OBJECTS) hf_5.obj hf_6.obj hf_64.obj hf_7.obj hf_8.obj hf_9.obj
OBJECTS = $(OBJECTS) mr2hcII_32.obj mr2hcII_64.obj mr2hc_128.obj mr2hc_32.obj
OBJECTS = $(OBJECTS) mr2hc_64.obj r2hcII_10.obj r2hcII_12.obj r2hcII_15.obj
OBJECTS = $(OBJECTS) r2hcII_16.obj r2hcII_2.obj r2hcII_3.obj r2hcII_32.obj
OBJECTS = $(OBJECTS) r2hcII_4.obj r2hcII_5.obj r2hcII_6.obj r2hcII_7.obj
OBJECTS = $(OBJECTS) r2hcII_8.obj r2hcII_9.obj r2hc_10.obj r2hc_11.obj r2hc_12.obj
OBJECTS = $(OBJECTS) r2hc_13.obj r2hc_14.obj r2hc_15.obj r2hc_16.obj r2hc_2.obj
OBJECTS = $(OBJECTS) r2hc_3.obj r2hc_32.obj r2hc_4.obj r2hc_5.obj r2hc_6.obj
OBJECTS = $(OBJECTS) r2hc_7.obj r2hc_8.obj r2hc_9.obj r2hc_codlist.obj

r2hc.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del r2hc.lib $(OBJECTS) 2> nul
