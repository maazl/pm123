#
#  api.lib makefile
#

DLL      = yes
TOPDIR   = ..
INCDIR   = $(TOPDIR);$(TOPDIR)\kernel;$(TOPDIR)\dft;$(TOPDIR)\rdft;$(TOPDIR)\reodft

!include $(TOPDIR)\..\config\makerules

all: api.lib $(MDUMMY)

OBJECTS1 = apiplan.obj configure.obj execute_dft_c2r.obj execute_dft_r2c.obj
OBJECTS1 = $(OBJECTS1) execute_dft.obj execute_r2r.obj execute_split_dft_c2r.obj
OBJECTS1 = $(OBJECTS1) execute_split_dft_r2c.obj execute_split_dft.obj execute.obj
OBJECTS1 = $(OBJECTS1) export_wisdom_to_file.obj export_wisdom_to_string.obj
OBJECTS1 = $(OBJECTS1) export_wisdom.obj extract_reim.obj flops.obj
OBJECTS1 = $(OBJECTS1) forget_wisdom.obj import_system_wisdom.obj
OBJECTS1 = $(OBJECTS1) import_wisdom_from_file.obj
OBJECTS1 = $(OBJECTS1) import_wisdom_from_string.obj import_wisdom.obj
OBJECTS1 = $(OBJECTS1) map_r2r_kind.obj mapflags.obj mkprinter_file.obj
OBJECTS1 = $(OBJECTS1) mktensor_iodims.obj mktensor_rowmajor.obj
OBJECTS1 = $(OBJECTS1) plan_dft_1d.obj plan_dft_2d.obj plan_dft_3d.obj
OBJECTS1 = $(OBJECTS1) plan_dft_c2r_1d.obj plan_dft_c2r_2d.obj

OBJECTS2 = plan_dft_c2r_3d.obj plan_dft_c2r.obj
OBJECTS2 = $(OBJECTS2) plan_dft_r2c_1d.obj plan_dft_r2c_2d.obj
OBJECTS2 = $(OBJECTS2) plan_dft_r2c_3d.obj plan_dft_r2c.obj plan_dft.obj
OBJECTS2 = $(OBJECTS2) plan_guru_dft_c2r.obj plan_guru_dft_r2c.obj
OBJECTS2 = $(OBJECTS2) plan_guru_dft.obj plan_guru_r2r.obj
OBJECTS2 = $(OBJECTS2) plan_guru_split_dft_c2r.obj plan_guru_split_dft_r2c.obj
OBJECTS2 = $(OBJECTS2) plan_guru_split_dft.obj plan_many_dft_c2r.obj
OBJECTS2 = $(OBJECTS2) plan_many_dft_r2c.obj plan_many_dft.obj plan_many_r2r.obj
OBJECTS2 = $(OBJECTS2) plan_r2r_1d.obj plan_r2r_2d.obj plan_r2r_3d.obj
OBJECTS2 = $(OBJECTS2) plan_r2r.obj print_plan.obj rdft2_pad.obj the_planner.obj
OBJECTS2 = $(OBJECTS2) version.obj

api.lib: $(OBJECTS1) $(OBJECTS2)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS1) $(LBENDL)
	$(LB) $@ $(OBJECTS2) $(LBENDL)

clean: $(MDUMMY)
	-@del api.lib     2> nul
	-@del $(OBJECTS1) 2> nul
	-@del $(OBJECTS2) 2> nul
