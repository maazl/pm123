#
#  fft123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

DEF     = fft$(LDEF_SUFFX)
LIBS    = kernel\kernel.lib 
LIBS    = $(LIBS) dft\codelets\inplace\dft_inplace.lib
LIBS    = $(LIBS) dft\codelets\standard\dft_standard.lib
LIBS    = $(LIBS) dft\codelets\dft_codelets.lib
LIBS    = $(LIBS) dft\dft.lib 
LIBS    = $(LIBS) rdft\codelets\hc2r\hc2r.lib
LIBS    = $(LIBS) rdft\codelets\r2hc\r2hc.lib
LIBS    = $(LIBS) rdft\codelets\r2r\r2r.lib
LIBS    = $(LIBS) rdft\codelets\rdft_codelets.lib
LIBS    = $(LIBS) rdft\rdft.lib
LIBS    = $(LIBS) reodft\reodft.lib
LIBS    = $(LIBS) api\api.lib
OBJECTS = fft.obj

all: fft123.dll fft123.lib $(MDUMMY)

fft123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

fft123.lib: $(DEF)
	$(LI) $@ fft123.dll

kernel\kernel.lib:
	cd kernel
	@$(MAKE) $(MFLAGS)
	@cd ..

dft\codelets\inplace\dft_inplace.lib:
	cd dft\codelets\inplace
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

dft\codelets\standard\dft_standard.lib:
	cd dft\codelets\standard
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

dft\codelets\dft_codelets.lib:
	cd dft\codelets
	@$(MAKE) $(MFLAGS)
	@cd ..\..

dft\dft.lib:
	cd dft
	@$(MAKE) $(MFLAGS)
	@cd ..

rdft\codelets\hc2r\hc2r.lib:
	cd rdft\codelets\hc2r
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\r2hc\r2hc.lib:
	cd rdft\codelets\r2hc
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\r2r\r2r.lib:
	cd rdft\codelets\r2r
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

rdft\codelets\rdft_codelets.lib:
	cd rdft\codelets
	@$(MAKE) $(MFLAGS)
	@cd ..\..

rdft\rdft.lib:
	cd rdft
	@$(MAKE) $(MFLAGS)
	@cd..

reodft\reodft.lib:
	cd reodft
	@$(MAKE) $(MFLAGS)
	@cd..

api\api.lib:
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
	-@del fft123.dll fft123.lib $(OBJECTS) 2> nul