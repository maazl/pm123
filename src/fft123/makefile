#
#  fft123.dll makefile
#

DLL     = yes
TOPDIR  = ..
TARGET  = 2

!include $(TOPDIR)\config\makerules

DEF     = fft$(LDEF_SUFFX)
LIBS    = kernel\kernel$(LBO) 
LIBS    = $(LIBS) dft\codelets\inplace\dft_inplace$(LBO)
LIBS    = $(LIBS) dft\codelets\standard\dft_standard$(LBO)
LIBS    = $(LIBS) dft\codelets\dft_codelets$(LBO)
LIBS    = $(LIBS) dft\dft$(LBO) 
LIBS    = $(LIBS) rdft\codelets\hc2r\hc2r$(LBO)
LIBS    = $(LIBS) rdft\codelets\r2hc\r2hc$(LBO)
LIBS    = $(LIBS) rdft\codelets\r2r\r2r$(LBO)
LIBS    = $(LIBS) rdft\codelets\rdft_codelets$(LBO)
LIBS    = $(LIBS) rdft\rdft$(LBO)
LIBS    = $(LIBS) reodft\reodft$(LBO)
LIBS    = $(LIBS) api\api$(LBO)
OBJECTS = fft$(CO)

all2: fft123.dll fft123$(LBI) $(MDUMMY)

fft123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

fft123$(LBI): $(DEF)
	$(LI) $@ fft123.dll

kernel\kernel$(LBO):
	cd kernel
	@$(MAKE) $(MFLAGS)
	@cd ..

dft\codelets\inplace\dft_inplace$(LBO):
	cd dft\codelets\inplace
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

dft\codelets\standard\dft_standard$(LBO):
	cd dft\codelets\standard
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

dft\codelets\dft_codelets$(LBO):
	cd dft\codelets
	@$(MAKE) $(MFLAGS)
	@cd ..\..

dft\dft$(LBO):
	cd dft
	@$(MAKE) $(MFLAGS)
	@cd ..

rdft\codelets\hc2r\hc2r$(LBO):
	cd rdft\codelets\hc2r
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\r2hc\r2hc$(LBO):
	cd rdft\codelets\r2hc
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\r2r\r2r$(LBO):
	cd rdft\codelets\r2r
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\rdft_codelets$(LBO):
	cd rdft\codelets
	@$(MAKE) $(MFLAGS)
	@cd ..\..

rdft\rdft$(LBO):
	cd rdft
	@$(MAKE) $(MFLAGS)
	@cd..

reodft\reodft$(LBO):
	cd reodft
	@$(MAKE) $(MFLAGS)
	@cd..

api\api$(LBO):
	cd api
	@$(MAKE) $(MFLAGS)
	@cd..

clean: $(MDUMMY)
	cd kernel
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd dft\codelets\inplace
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd dft\codelets\standard
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd dft\codelets
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd dft
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd rdft\codelets\hc2r
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd rdft\codelets\r2hc
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd rdft\codelets\r2r
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd rdft\codelets
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd rdft
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd reodft
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd api
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	-@del fft123.dll fft123$(LBI) $(OBJECTS) 2> nul