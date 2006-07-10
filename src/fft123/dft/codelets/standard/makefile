#
#  dft_standard.lib makefile
#

DLL     = yes
TOPDIR  = ..\..\..
INCDIR  = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft;$(TOPDIR)\dft\codelets

!include $(TOPDIR)\..\config\makerules

all: dft_standard.lib $(MDUMMY)

OBJECTS = s_codlist.obj m1_16.obj m1_32.obj m1_64.obj n1_10.obj n1_11.obj n1_12.obj
OBJECTS = $(OBJECTS) n1_13.obj n1_14.obj n1_15.obj n1_16.obj n1_2.obj n1_3.obj
OBJECTS = $(OBJECTS) n1_4.obj n1_5.obj n1_6.obj n1_7.obj n1_8.obj n1_9.obj
OBJECTS = $(OBJECTS) t1_10.obj t1_12.obj t1_15.obj t1_16.obj t1_2.obj t1_3.obj
OBJECTS = $(OBJECTS) t1_32.obj t1_4.obj t1_5.obj t1_6.obj t1_64.obj t1_7.obj
OBJECTS = $(OBJECTS) t1_8.obj t1_9.obj t2_16.obj t2_32.obj t2_4.obj t2_64.obj
OBJECTS = $(OBJECTS) t2_8.obj

dft_standard.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del dft_standard.lib $(OBJECTS) 2> nul
