#
#  dft_inplace.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft;$(TOPDIR)\dft\codelets

!include $(TOPDIR)\..\config\makerules

all: dft_inplace.lib $(MDUMMY)

OBJECTS = i_codlist.obj q1_2.obj q1_3.obj q1_4.obj q1_5.obj q1_6.obj q1_8.obj

dft_inplace.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_inplace.lib $(OBJECTS) 2> nul
