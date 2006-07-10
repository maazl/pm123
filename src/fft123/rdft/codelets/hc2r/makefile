#
#  hc2r.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\rdft;$(TOPDIR)\rdft\codelets

!include $(TOPDIR)\..\config\makerules

all: hc2r.lib $(MDUMMY)

OBJECTS = hb_10.obj hb_12.obj hb_15.obj hb_16.obj hb_2.obj hb_3.obj hb_32.obj
OBJECTS = $(OBJECTS) hb_4.obj hb_5.obj hb_6.obj hb_64.obj hb_7.obj hb_8.obj
OBJECTS = $(OBJECTS) hb_9.obj hc2rIII_10.obj hc2rIII_12.obj hc2rIII_15.obj
OBJECTS = $(OBJECTS) hc2rIII_16.obj hc2rIII_2.obj hc2rIII_3.obj hc2rIII_32.obj
OBJECTS = $(OBJECTS) hc2rIII_4.obj hc2rIII_5.obj hc2rIII_6.obj hc2rIII_7.obj
OBJECTS = $(OBJECTS) hc2rIII_8.obj hc2rIII_9.obj hc2r_10.obj hc2r_11.obj
OBJECTS = $(OBJECTS) hc2r_12.obj hc2r_13.obj hc2r_14.obj hc2r_15.obj
OBJECTS = $(OBJECTS) hc2r_16.obj hc2r_3.obj hc2r_32.obj hc2r_4.obj hc2r_5.obj
OBJECTS = $(OBJECTS) hc2r_6.obj hc2r_7.obj hc2r_8.obj hc2r_9.obj
OBJECTS = $(OBJECTS) mhc2rIII_32.obj mhc2rIII_64.obj mhc2r_128.obj mhc2r_32.obj
OBJECTS = $(OBJECTS) mhc2r_64.obj hc2r_codlist.obj

hc2r.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del hc2r.lib $(OBJECTS) 2> nul
