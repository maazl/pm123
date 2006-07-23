#
#  dft_inplace library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft;$(TOPDIR)\dft\codelets

!include $(TOPDIR)\..\config\makerules

all: dft_inplace$(LBO) $(MDUMMY)

OBJECTS = i_codlist$(CO) q1_2$(CO) q1_3$(CO) q1_4$(CO) q1_5$(CO) q1_6$(CO) q1_8$(CO)

dft_inplace$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_inplace$(LBO) $(OBJECTS) 2> nul
