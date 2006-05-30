# IBM Visualage C++ makefile

!include ..\config\makerules

CFLAGS  = $(CFLAGS) $(DLLFLG) -Wcnd-use-
INCLUDE = .;kernel;dft;dft\codelets;rdft;rdft\codelets;reodft;$(INCLUDE)
LIB     = $(LIB)

all: fft123.dll

OBJECTS = kernel\align.obj kernel\alloc.obj kernel\assert.obj kernel\awake.obj\
          kernel\buffered.obj kernel\ct.obj kernel\debug.obj kernel\hash.obj\
          kernel\iabs.obj kernel\md5-1.obj kernel\md5.obj kernel\minmax.obj\
          kernel\ops.obj kernel\pickdim.obj kernel\plan.obj kernel\planner.obj\
          kernel\primes.obj kernel\print.obj kernel\problem.obj kernel\rader.obj\
          kernel\scan.obj kernel\solver.obj kernel\solvtab.obj kernel\square.obj\
          kernel\stride.obj kernel\tensor.obj kernel\tensor1.obj kernel\tensor2.obj\
          kernel\tensor4.obj kernel\tensor5.obj kernel\tensor7.obj kernel\tensor8.obj\
          kernel\tensor9.obj kernel\timer.obj kernel\transpose.obj kernel\trig.obj\
          kernel\trig1.obj kernel\twiddle.obj

OBJECTS = $(OBJECTS)\
          dft\buffered.obj dft\conf.obj dft\ct-dif.obj dft\ct-dit.obj\
          dft\ct-ditbuf.obj dft\ct-ditf.obj dft\ct.obj dft\direct.obj dft\generic.obj\
          dft\indirect.obj dft\kdft-dif.obj dft\kdft-difsq.obj dft\kdft-dit.obj\
          dft\kdft.obj dft\nop.obj dft\plan.obj dft\problem.obj dft\rader-omega.obj\
          dft\rader.obj dft\rank-geq2.obj dft\rank0.obj dft\solve.obj\
          dft\vrank-geq1.obj dft\vrank2-transpose.obj dft\vrank3-transpose.obj\
          dft\zero.obj 

OBJECTS = $(OBJECTS)\
          dft\codelets\n.obj dft\codelets\t.obj\
          dft\codelets\inplace\codlist.obj dft\codelets\inplace\q1_2.obj\
          dft\codelets\inplace\q1_3.obj dft\codelets\inplace\q1_4.obj\
          dft\codelets\inplace\q1_5.obj dft\codelets\inplace\q1_6.obj\
          dft\codelets\inplace\q1_8.obj\
          dft\codelets\standard\codlist.obj dft\codelets\standard\m1_16.obj\
          dft\codelets\standard\m1_32.obj dft\codelets\standard\m1_64.obj\
          dft\codelets\standard\n1_10.obj dft\codelets\standard\n1_11.obj\
          dft\codelets\standard\n1_12.obj dft\codelets\standard\n1_13.obj\
          dft\codelets\standard\n1_14.obj dft\codelets\standard\n1_15.obj\
          dft\codelets\standard\n1_16.obj dft\codelets\standard\n1_2.obj\
          dft\codelets\standard\n1_3.obj dft\codelets\standard\n1_4.obj\
          dft\codelets\standard\n1_5.obj dft\codelets\standard\n1_6.obj\
          dft\codelets\standard\n1_7.obj dft\codelets\standard\n1_8.obj\
          dft\codelets\standard\n1_9.obj dft\codelets\standard\t1_10.obj\
          dft\codelets\standard\t1_12.obj dft\codelets\standard\t1_15.obj\
          dft\codelets\standard\t1_16.obj dft\codelets\standard\t1_2.obj\
          dft\codelets\standard\t1_3.obj dft\codelets\standard\t1_32.obj\
          dft\codelets\standard\t1_4.obj dft\codelets\standard\t1_5.obj\
          dft\codelets\standard\t1_6.obj dft\codelets\standard\t1_64.obj\
          dft\codelets\standard\t1_7.obj dft\codelets\standard\t1_8.obj\
          dft\codelets\standard\t1_9.obj dft\codelets\standard\t2_16.obj\
          dft\codelets\standard\t2_32.obj dft\codelets\standard\t2_4.obj\
          dft\codelets\standard\t2_64.obj dft\codelets\standard\t2_8.obj

OBJECTS = $(OBJECTS)\
          rdft\buffered.obj rdft\buffered2.obj rdft\conf.obj rdft\dft-r2hc.obj\
          rdft\dht-r2hc.obj rdft\dht-rader.obj rdft\direct.obj rdft\direct2.obj\
          rdft\generic.obj rdft\hc2hc-buf.obj rdft\hc2hc-dif.obj rdft\hc2hc-dit.obj\
          rdft\hc2hc.obj rdft\indirect.obj rdft\khc2hc-dif.obj rdft\khc2hc-dit.obj\
          rdft\khc2r.obj rdft\kr2hc.obj rdft\kr2r.obj rdft\nop.obj rdft\nop2.obj\
          rdft\plan.obj rdft\plan2.obj rdft\problem.obj rdft\problem2.obj rdft\rader-hc2hc.obj\
          rdft\rank-geq2-rdft2.obj rdft\rank-geq2.obj rdft\rank0-rdft2.obj rdft\rank0.obj\
          rdft\rdft-dht.obj rdft\rdft2-inplace-strides.obj rdft\rdft2-radix2.obj\
          rdft\rdft2-strides.obj rdft\rdft2-tensor-max-index.obj rdft\solve.obj\
          rdft\solve2.obj rdft\vrank-geq1-rdft2.obj rdft\vrank-geq1.obj

OBJECTS = $(OBJECTS)\
          rdft\codelets\hc2r.obj rdft\codelets\hfb.obj rdft\codelets\r2hc.obj\
          rdft\codelets\r2r.obj\
          rdft\codelets\hc2r\codlist.obj rdft\codelets\hc2r\hb_10.obj\
          rdft\codelets\hc2r\hb_12.obj rdft\codelets\hc2r\hb_15.obj\
          rdft\codelets\hc2r\hb_16.obj rdft\codelets\hc2r\hb_2.obj\
          rdft\codelets\hc2r\hb_3.obj rdft\codelets\hc2r\hb_32.obj\
          rdft\codelets\hc2r\hb_4.obj rdft\codelets\hc2r\hb_5.obj\
          rdft\codelets\hc2r\hb_6.obj rdft\codelets\hc2r\hb_64.obj\
          rdft\codelets\hc2r\hb_7.obj rdft\codelets\hc2r\hb_8.obj\
          rdft\codelets\hc2r\hb_9.obj rdft\codelets\hc2r\hc2rIII_10.obj\
          rdft\codelets\hc2r\hc2rIII_12.obj rdft\codelets\hc2r\hc2rIII_15.obj\
          rdft\codelets\hc2r\hc2rIII_16.obj rdft\codelets\hc2r\hc2rIII_2.obj\
          rdft\codelets\hc2r\hc2rIII_3.obj rdft\codelets\hc2r\hc2rIII_32.obj\
          rdft\codelets\hc2r\hc2rIII_4.obj rdft\codelets\hc2r\hc2rIII_5.obj\
          rdft\codelets\hc2r\hc2rIII_6.obj rdft\codelets\hc2r\hc2rIII_7.obj\
          rdft\codelets\hc2r\hc2rIII_8.obj rdft\codelets\hc2r\hc2rIII_9.obj\
          rdft\codelets\hc2r\hc2r_10.obj rdft\codelets\hc2r\hc2r_11.obj\
          rdft\codelets\hc2r\hc2r_12.obj rdft\codelets\hc2r\hc2r_13.obj\
          rdft\codelets\hc2r\hc2r_14.obj rdft\codelets\hc2r\hc2r_15.obj\
          rdft\codelets\hc2r\hc2r_16.obj rdft\codelets\hc2r\hc2r_3.obj\
          rdft\codelets\hc2r\hc2r_32.obj rdft\codelets\hc2r\hc2r_4.obj\
          rdft\codelets\hc2r\hc2r_5.obj rdft\codelets\hc2r\hc2r_6.obj\
          rdft\codelets\hc2r\hc2r_7.obj rdft\codelets\hc2r\hc2r_8.obj\
          rdft\codelets\hc2r\hc2r_9.obj rdft\codelets\hc2r\mhc2rIII_32.obj\
          rdft\codelets\hc2r\mhc2rIII_64.obj rdft\codelets\hc2r\mhc2r_128.obj\
          rdft\codelets\hc2r\mhc2r_32.obj rdft\codelets\hc2r\mhc2r_64.obj\
          rdft\codelets\r2hc\codlist.obj rdft\codelets\r2hc\hf2_16.obj\
          rdft\codelets\r2hc\hf2_32.obj rdft\codelets\r2hc\hf2_4.obj\
          rdft\codelets\r2hc\hf2_64.obj rdft\codelets\r2hc\hf2_8.obj\
          rdft\codelets\r2hc\hf_10.obj rdft\codelets\r2hc\hf_12.obj\
          rdft\codelets\r2hc\hf_15.obj rdft\codelets\r2hc\hf_16.obj\
          rdft\codelets\r2hc\hf_2.obj rdft\codelets\r2hc\hf_3.obj\
          rdft\codelets\r2hc\hf_32.obj rdft\codelets\r2hc\hf_4.obj\
          rdft\codelets\r2hc\hf_5.obj rdft\codelets\r2hc\hf_6.obj\
          rdft\codelets\r2hc\hf_64.obj rdft\codelets\r2hc\hf_7.obj\
          rdft\codelets\r2hc\hf_8.obj rdft\codelets\r2hc\hf_9.obj\
          rdft\codelets\r2hc\mr2hcII_32.obj rdft\codelets\r2hc\mr2hcII_64.obj\
          rdft\codelets\r2hc\mr2hc_128.obj rdft\codelets\r2hc\mr2hc_32.obj\
          rdft\codelets\r2hc\mr2hc_64.obj rdft\codelets\r2hc\r2hcII_10.obj\
          rdft\codelets\r2hc\r2hcII_12.obj rdft\codelets\r2hc\r2hcII_15.obj\
          rdft\codelets\r2hc\r2hcII_16.obj rdft\codelets\r2hc\r2hcII_2.obj\
          rdft\codelets\r2hc\r2hcII_3.obj rdft\codelets\r2hc\r2hcII_32.obj\
          rdft\codelets\r2hc\r2hcII_4.obj rdft\codelets\r2hc\r2hcII_5.obj\
          rdft\codelets\r2hc\r2hcII_6.obj rdft\codelets\r2hc\r2hcII_7.obj\
          rdft\codelets\r2hc\r2hcII_8.obj rdft\codelets\r2hc\r2hcII_9.obj\
          rdft\codelets\r2hc\r2hc_10.obj rdft\codelets\r2hc\r2hc_11.obj\
          rdft\codelets\r2hc\r2hc_12.obj rdft\codelets\r2hc\r2hc_13.obj\
          rdft\codelets\r2hc\r2hc_14.obj rdft\codelets\r2hc\r2hc_15.obj\
          rdft\codelets\r2hc\r2hc_16.obj rdft\codelets\r2hc\r2hc_2.obj\
          rdft\codelets\r2hc\r2hc_3.obj rdft\codelets\r2hc\r2hc_32.obj\
          rdft\codelets\r2hc\r2hc_4.obj rdft\codelets\r2hc\r2hc_5.obj\
          rdft\codelets\r2hc\r2hc_6.obj rdft\codelets\r2hc\r2hc_7.obj\
          rdft\codelets\r2hc\r2hc_8.obj rdft\codelets\r2hc\r2hc_9.obj\
          rdft\codelets\r2r\codlist.obj rdft\codelets\r2r\e01_8.obj\
          rdft\codelets\r2r\e10_8.obj

OBJECTS = $(OBJECTS)\
          reodft\conf.obj reodft\redft00e-r2hc-pad.obj reodft\redft00e-r2hc.obj\
          reodft\reodft010e-r2hc.obj reodft\reodft11e-r2hc-odd.obj\
          reodft\reodft11e-r2hc.obj reodft\reodft11e-radix2.obj\
          reodft\rodft00e-r2hc-pad.obj reodft\rodft00e-r2hc.obj
 
OBJECTS = $(OBJECTS)\
          api\apiplan.obj api\configure.obj api\execute-dft-c2r.obj api\execute-dft-r2c.obj\
          api\execute-dft.obj api\execute-r2r.obj api\execute-split-dft-c2r.obj\
          api\execute-split-dft-r2c.obj api\execute-split-dft.obj api\execute.obj\
          api\export-wisdom-to-file.obj api\export-wisdom-to-string.obj\
          api\export-wisdom.obj api\extract-reim.obj api\f77api.obj api\flops.obj\
          api\forget-wisdom.obj api\import-system-wisdom.obj api\import-wisdom-from-file.obj\
          api\import-wisdom-from-string.obj api\import-wisdom.obj api\map-r2r-kind.obj\
          api\mapflags.obj api\mkprinter-file.obj api\mktensor-iodims.obj\
          api\mktensor-rowmajor.obj api\plan-dft-1d.obj api\plan-dft-2d.obj api\plan-dft-3d.obj\
          api\plan-dft-c2r-1d.obj api\plan-dft-c2r-2d.obj api\plan-dft-c2r-3d.obj\
          api\plan-dft-c2r.obj api\plan-dft-r2c-1d.obj api\plan-dft-r2c-2d.obj\
          api\plan-dft-r2c-3d.obj api\plan-dft-r2c.obj api\plan-dft.obj\
          api\plan-guru-dft-c2r.obj api\plan-guru-dft-r2c.obj api\plan-guru-dft.obj\
          api\plan-guru-r2r.obj api\plan-guru-split-dft-c2r.obj api\plan-guru-split-dft-r2c.obj\
          api\plan-guru-split-dft.obj api\plan-many-dft-c2r.obj api\plan-many-dft-r2c.obj\
          api\plan-many-dft.obj api\plan-many-r2r.obj api\plan-r2r-1d.obj api\plan-r2r-2d.obj\
          api\plan-r2r-3d.obj api\plan-r2r.obj api\print-plan.obj api\rdft2-pad.obj\
          api\the-planner.obj api\version.obj 

DEF     = fft123.def

fft123.dll: $(OBJECTS) $(DEF)
   $(CC) $(LNKFLG) /Fm$* /Fe$@ $(LIBS) @<<
   $(**: = ^
)
<<
   $(LI) $*.lib $@

clean:
  -@del fft123.dll fft123.lib fft123.map *.obj 2> nul
   cd kernel
  -@del *.obj 2> nul
   @cd ..
   cd dft
  -@del *.obj 2> nul
   @cd ..
   cd dft\codelets
  -@del *.obj 2> nul
   @cd ..\..
   cd dft\codelets\inplace
  -@del *.obj 2> nul
   @cd ..\..\..
   cd dft\codelets\standard
  -@del *.obj 2> nul
   @cd ..\..\..
   cd rdft
  -@del *.obj 2> nul
   @cd ..
   cd rdft\codelets
  -@del *.obj 2> nul
   @cd ..\..
   cd rdft\codelets\hc2r
  -@del *.obj 2> nul
   @cd ..\..\..
   cd rdft\codelets\r2hc
  -@del *.obj 2> nul
   @cd ..\..\..
   cd rdft\codelets\r2r
  -@del *.obj 2> nul
   @cd ..\..\..
   cd reodft
  -@del *.obj 2> nul
   @cd ..
   cd api
  -@del *.obj 2> nul
   @cd ..
