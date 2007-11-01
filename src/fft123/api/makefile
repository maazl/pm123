#
#  api library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = ..\kernel;..\dft;..\rdft;..\reodft;$(TOPDIR)\include
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: api$(LBO) $(MDUMMY)

OBJECTS1 = apiplan$(CO) configure$(CO) execute_dft_c2r$(CO) execute_dft_r2c$(CO)
OBJECTS1 = $(OBJECTS1) execute_dft$(CO) execute_r2r$(CO) execute_split_dft_c2r$(CO)
OBJECTS1 = $(OBJECTS1) execute_split_dft_r2c$(CO) execute_split_dft$(CO) execute$(CO)
OBJECTS1 = $(OBJECTS1) export_wisdom_to_file$(CO) export_wisdom_to_string$(CO)
OBJECTS1 = $(OBJECTS1) export_wisdom$(CO) extract_reim$(CO) flops$(CO)
OBJECTS1 = $(OBJECTS1) forget_wisdom$(CO) import_system_wisdom$(CO)
OBJECTS1 = $(OBJECTS1) import_wisdom_from_file$(CO)
OBJECTS1 = $(OBJECTS1) import_wisdom_from_string$(CO) import_wisdom$(CO)
OBJECTS1 = $(OBJECTS1) map_r2r_kind$(CO) mapflags$(CO) mkprinter_file$(CO)
OBJECTS1 = $(OBJECTS1) mktensor_iodims$(CO) mktensor_rowmajor$(CO)
OBJECTS1 = $(OBJECTS1) plan_dft_1d$(CO) plan_dft_2d$(CO) plan_dft_3d$(CO)
OBJECTS1 = $(OBJECTS1) plan_dft_c2r_1d$(CO) plan_dft_c2r_2d$(CO)

OBJECTS2 = plan_dft_c2r_3d$(CO) plan_dft_c2r$(CO)
OBJECTS2 = $(OBJECTS2) plan_dft_r2c_1d$(CO) plan_dft_r2c_2d$(CO)
OBJECTS2 = $(OBJECTS2) plan_dft_r2c_3d$(CO) plan_dft_r2c$(CO) plan_dft$(CO)
OBJECTS2 = $(OBJECTS2) plan_guru_dft_c2r$(CO) plan_guru_dft_r2c$(CO)
OBJECTS2 = $(OBJECTS2) plan_guru_dft$(CO) plan_guru_r2r$(CO)
OBJECTS2 = $(OBJECTS2) plan_guru_split_dft_c2r$(CO) plan_guru_split_dft_r2c$(CO)
OBJECTS2 = $(OBJECTS2) plan_guru_split_dft$(CO) plan_many_dft_c2r$(CO)
OBJECTS2 = $(OBJECTS2) plan_many_dft_r2c$(CO) plan_many_dft$(CO) plan_many_r2r$(CO)
OBJECTS2 = $(OBJECTS2) plan_r2r_1d$(CO) plan_r2r_2d$(CO) plan_r2r_3d$(CO)
OBJECTS2 = $(OBJECTS2) plan_r2r$(CO) print_plan$(CO) rdft2_pad$(CO) the_planner$(CO)
OBJECTS2 = $(OBJECTS2) version$(CO)

api$(LBO): $(OBJECTS1) $(OBJECTS2)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS1) $(LBENDL)
	$(LB) $@ $(OBJECTS2) $(LBENDL)

clean: $(MDUMMY)
	-@del api$(LBO)     2> nul
	-@del $(OBJECTS1) 2> nul
	-@del $(OBJECTS2) 2> nul
