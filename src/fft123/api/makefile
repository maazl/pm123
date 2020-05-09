#
#  api library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = ..\kernel;..\dft;..\rdft;..\reodft;$(TOPDIR)\include;$(TOPDIR)\utils
TARGET   = all2

include $(TOPDIR)\config\makerules

all2: api$(LBO) $(MDUMMY)

OBJECTS1 = apiplan$(CO) configure$(CO) execute_dft_c2r$(CO) execute_dft_r2c$(CO) \
	execute_dft$(CO) execute_r2r$(CO) execute_split_dft_c2r$(CO) \
	execute_split_dft_r2c$(CO) execute_split_dft$(CO) execute$(CO) \
	export_wisdom_to_file$(CO) export_wisdom_to_string$(CO) \
	export_wisdom$(CO) extract_reim$(CO) flops$(CO) \
	forget_wisdom$(CO) import_system_wisdom$(CO) \
	import_wisdom_from_file$(CO) import_wisdom_from_string$(CO) import_wisdom$(CO) \
	map_r2r_kind$(CO) mapflags$(CO) mkprinter_file$(CO) \
	mktensor_iodims$(CO) mktensor_rowmajor$(CO) \
	plan_dft_1d$(CO) plan_dft_2d$(CO) plan_dft_3d$(CO) \
	plan_dft_c2r_1d$(CO) plan_dft_c2r_2d$(CO)

OBJECTS2 = plan_dft_c2r_3d$(CO) plan_dft_c2r$(CO) \
	plan_dft_r2c_1d$(CO) plan_dft_r2c_2d$(CO) \
	plan_dft_r2c_3d$(CO) plan_dft_r2c$(CO) plan_dft$(CO) \
	plan_guru_dft_c2r$(CO) plan_guru_dft_r2c$(CO) \
	plan_guru_dft$(CO) plan_guru_r2r$(CO) \
	plan_guru_split_dft_c2r$(CO) plan_guru_split_dft_r2c$(CO) \
	plan_guru_split_dft$(CO) plan_many_dft_c2r$(CO) \
	plan_many_dft_r2c$(CO) plan_many_dft$(CO) plan_many_r2r$(CO) \
	plan_r2r_1d$(CO) plan_r2r_2d$(CO) plan_r2r_3d$(CO) \
	plan_r2r$(CO) print_plan$(CO) rdft2_pad$(CO) the_planner$(CO) \
	version$(CO) synchronize$(CO)

api$(LBO): $(OBJECTS1) $(OBJECTS2)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS1) $(LBENDL)
	$(LB) $@ $(OBJECTS2) $(LBENDL)

clean: $(MDUMMY)
	-@del api$(LBO)     2> nul
	-@del $(OBJECTS1) 2> nul
	-@del $(OBJECTS2) 2> nul
