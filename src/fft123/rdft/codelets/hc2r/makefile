#
#  hc2r library makefile
#

DLL     = yes
TOPDIR  = ..\..\..\..
INCDIR  = ..\..\..\kernel;..\..\..\rdft;..\..\..\rdft\codelets;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

all: hc2r$(LBO) $(MDUMMY)

OBJECTS = hb_10$(CO) hb_12$(CO) hb_15$(CO) hb_16$(CO) hb_2$(CO) hb_3$(CO) hb_32$(CO)
OBJECTS = $(OBJECTS) hb_4$(CO) hb_5$(CO) hb_6$(CO) hb_64$(CO) hb_7$(CO) hb_8$(CO)
OBJECTS = $(OBJECTS) hb_9$(CO) hc2rIII_10$(CO) hc2rIII_12$(CO) hc2rIII_15$(CO)
OBJECTS = $(OBJECTS) hc2rIII_16$(CO) hc2rIII_2$(CO) hc2rIII_3$(CO) hc2rIII_32$(CO)
OBJECTS = $(OBJECTS) hc2rIII_4$(CO) hc2rIII_5$(CO) hc2rIII_6$(CO) hc2rIII_7$(CO)
OBJECTS = $(OBJECTS) hc2rIII_8$(CO) hc2rIII_9$(CO) hc2r_10$(CO) hc2r_11$(CO)
OBJECTS = $(OBJECTS) hc2r_12$(CO) hc2r_13$(CO) hc2r_14$(CO) hc2r_15$(CO)
OBJECTS = $(OBJECTS) hc2r_16$(CO) hc2r_3$(CO) hc2r_32$(CO) hc2r_4$(CO) hc2r_5$(CO)
OBJECTS = $(OBJECTS) hc2r_6$(CO) hc2r_7$(CO) hc2r_8$(CO) hc2r_9$(CO)
OBJECTS = $(OBJECTS) mhc2rIII_32$(CO) mhc2rIII_64$(CO) mhc2r_128$(CO) mhc2r_32$(CO)
OBJECTS = $(OBJECTS) mhc2r_64$(CO) hc2r_codlist$(CO)

hc2r$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del hc2r$(LBO) $(OBJECTS) 2> nul
