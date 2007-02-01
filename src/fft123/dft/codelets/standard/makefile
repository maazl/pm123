#
#  dft_standard library makefile
#

DLL     = yes
TOPDIR  = ..\..\..\..
INCDIR  = ..\..\..\kernel;..\..\..\dft;..\..\..\dft\codelets;$(TOPDIR)\include

!include $(TOPDIR)\config\makerules

all: dft_standard$(LBO) $(MDUMMY)

OBJECTS = s_codlist$(CO) m1_16$(CO) m1_32$(CO) m1_64$(CO) n1_10$(CO) n1_11$(CO) n1_12$(CO)
OBJECTS = $(OBJECTS) n1_13$(CO) n1_14$(CO) n1_15$(CO) n1_16$(CO) n1_2$(CO) n1_3$(CO)
OBJECTS = $(OBJECTS) n1_4$(CO) n1_5$(CO) n1_6$(CO) n1_7$(CO) n1_8$(CO) n1_9$(CO)
OBJECTS = $(OBJECTS) t1_10$(CO) t1_12$(CO) t1_15$(CO) t1_16$(CO) t1_2$(CO) t1_3$(CO)
OBJECTS = $(OBJECTS) t1_32$(CO) t1_4$(CO) t1_5$(CO) t1_6$(CO) t1_64$(CO) t1_7$(CO)
OBJECTS = $(OBJECTS) t1_8$(CO) t1_9$(CO) t2_16$(CO) t2_32$(CO) t2_4$(CO) t2_64$(CO)
OBJECTS = $(OBJECTS) t2_8$(CO)

dft_standard$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_standard$(LBO) $(OBJECTS) 2> nul
