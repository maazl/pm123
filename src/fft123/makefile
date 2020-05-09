#
#  fft123.dll makefile
#

DLL     = yes
TOPDIR  = ..
TARGET  = all2

include $(TOPDIR)\config\makerules

DEF     = fft$(LDEF_SUFFX)
LIBS    = kernel\kernel$(LBO) \
	dft\codelets\inplace\dft_inplace$(LBO) \
	dft\codelets\standard\dft_standard$(LBO) \
	dft\codelets\dft_codelets$(LBO) \
	dft\dft$(LBO) \
	rdft\codelets\hc2r\hc2r$(LBO) \
	rdft\codelets\r2hc\r2hc$(LBO) \
	rdft\codelets\r2r\r2r$(LBO) \
	rdft\codelets\rdft_codelets$(LBO) \
	rdft\rdft$(LBO) \
	reodft\reodft$(LBO) \
	api\api$(LBO) \
	$(TOPDIR)\utils\utilfct$(LBO)
OBJECTS = fft$(CO)

all2: fft123.dll fft123$(LBI) $(MDUMMY)

fft123.dll fft123$(LBI): $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(LI) $(IFLAGS_OUT)fft123$(LBI) fft123.dll
	$(POSTLINKCMD)

kernel\kernel$(LBO):
	@$(MAKE) -C kernel $(MFLAGS)

dft\codelets\inplace\dft_inplace$(LBO):
	@$(MAKE) -C dft\codelets\inplace $(MFLAGS)

dft\codelets\standard\dft_standard$(LBO):
	@$(MAKE) -C dft\codelets\standard $(MFLAGS)

dft\codelets\dft_codelets$(LBO):
	@$(MAKE) -C dft\codelets $(MFLAGS)

dft\dft$(LBO):
	@$(MAKE) -C dft $(MFLAGS)

rdft\codelets\hc2r\hc2r$(LBO):
	@$(MAKE) -C rdft\codelets\hc2r $(MFLAGS)

rdft\codelets\r2hc\r2hc$(LBO):
	@$(MAKE) -C rdft\codelets\r2hc $(MFLAGS)

rdft\codelets\r2r\r2r$(LBO):
	@$(MAKE) -C rdft\codelets\r2r $(MFLAGS)

rdft\codelets\rdft_codelets$(LBO):
	@$(MAKE) -C rdft\codelets $(MFLAGS)

rdft\rdft$(LBO):
	@$(MAKE) -C rdft $(MFLAGS)

reodft\reodft$(LBO):
	@$(MAKE) -C reodft $(MFLAGS)

api\api$(LBO):
	@$(MAKE) -C api $(MFLAGS)

clean: $(MDUMMY)
	@$(MAKE) -C kernel $(MFLAGS) clean
	@$(MAKE) -C dft\codelets\inplace $(MFLAGS) clean
	@$(MAKE) -C dft\codelets\standard $(MFLAGS) clean
	@$(MAKE) -C dft\codelets $(MFLAGS) clean
	@$(MAKE) -C dft $(MFLAGS) clean
	@$(MAKE) -C rdft\codelets\hc2r $(MFLAGS) clean
	@$(MAKE) -C rdft\codelets\r2hc $(MFLAGS) clean
	@$(MAKE) -C rdft\codelets\r2r $(MFLAGS) clean
	@$(MAKE) -C rdft\codelets $(MFLAGS) clean
	@$(MAKE) -C rdft $(MFLAGS) clean
	@$(MAKE) -C reodft $(MFLAGS) clean
	@$(MAKE) -C api $(MFLAGS) clean
	-@del fft123.dll fft123$(LBI) $(OBJECTS) 2> nul
