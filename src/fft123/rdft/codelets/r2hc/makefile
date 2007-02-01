#
#  r2hc library makefile
#

DLL     = yes
TOPDIR  = ..\..\..\..
INCDIR  = ..\..\..\kernel;..\..\..\rdft;..\..\..\rdft\codelets;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

all: r2hc$(LBO) $(MDUMMY)

OBJECTS = hf2_16$(CO) hf2_32$(CO) hf2_4$(CO) hf2_64$(CO) hf2_8$(CO) hf_10$(CO) hf_12$(CO)
OBJECTS = $(OBJECTS) hf_15$(CO) hf_16$(CO) hf_2$(CO) hf_3$(CO) hf_32$(CO) hf_4$(CO)
OBJECTS = $(OBJECTS) hf_5$(CO) hf_6$(CO) hf_64$(CO) hf_7$(CO) hf_8$(CO) hf_9$(CO)
OBJECTS = $(OBJECTS) mr2hcII_32$(CO) mr2hcII_64$(CO) mr2hc_128$(CO) mr2hc_32$(CO)
OBJECTS = $(OBJECTS) mr2hc_64$(CO) r2hcII_10$(CO) r2hcII_12$(CO) r2hcII_15$(CO)
OBJECTS = $(OBJECTS) r2hcII_16$(CO) r2hcII_2$(CO) r2hcII_3$(CO) r2hcII_32$(CO)
OBJECTS = $(OBJECTS) r2hcII_4$(CO) r2hcII_5$(CO) r2hcII_6$(CO) r2hcII_7$(CO)
OBJECTS = $(OBJECTS) r2hcII_8$(CO) r2hcII_9$(CO) r2hc_10$(CO) r2hc_11$(CO) r2hc_12$(CO)
OBJECTS = $(OBJECTS) r2hc_13$(CO) r2hc_14$(CO) r2hc_15$(CO) r2hc_16$(CO) r2hc_2$(CO)
OBJECTS = $(OBJECTS) r2hc_3$(CO) r2hc_32$(CO) r2hc_4$(CO) r2hc_5$(CO) r2hc_6$(CO)
OBJECTS = $(OBJECTS) r2hc_7$(CO) r2hc_8$(CO) r2hc_9$(CO) r2hc_codlist$(CO)

r2hc$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del r2hc$(LBO) $(OBJECTS) 2> nul
