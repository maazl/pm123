#
# Generalised Bitmap Module
#
# OS/2
# IBM VisualAge C++, with CTC306, CTD302 and CTU304 or FP8
#
# Builds GBM.DLL which is compiled multithreaded
# Therefore all users should also be multithreaded
#
# Note problems have been observed when optimisation allows the instruction
# scheduler to do its thing. Bug present beleived present in IBM C-Set++, with
# CSD 12, and in particular affects inner loops in gbmht.c and gbmerr.c.
# Bug suspected also present in VisualAge C++, even with CTC305.
#

!include ..\config\makerules

CWARNS         = /W3 /Wuse+
CFLAGS         = /C+ $(CWARNS) /DOS2 /G5 /Gd-m+ /Q+ /O+ /Os- /Gf+  $(CJPEG) $(CPNG)
CFLAGS_DBG     = /C+ $(CWARNS) /DOS2 /G5 /Gd-m+ /Q+ /Gf+ $(CJPEG) $(CPNG) /Ti+ /Tm+ /DEbug
CFLAGS_EXE     = $(CFLAGS) /Ge+
CFLAGS_EXE_DBG = $(CFLAGS_DBG) /Ge+
CFLAGS_DLL     = $(CFLAGS) /Ge-
CFLAGS_DLL_DBG = $(CFLAGS_DBG) /Ge- 
LFLAGS         = /NOI /ALIGN:4 /NOLOGO /EXEPACK:2 /OPTFUNC /PACKCODE /PACKDATA
LFLAGS_DBG     = /NOI /ALIGN:4 /NOLOGO /DEBUG /DBGPACK
LFLAGS_EXE     = $(LFLAGS) /BASE:0x10000 /STACK:0x6000 /PMTYPE:VIO
LFLAGS_EXE_DBG = $(LFLAGS_DBG) /BASE:0x10000 /STACK:0x6000 /PMTYPE:VIO
LFLAGS_DLL     = $(LFLAGS) /NOE /DLL
LFLAGS_DLL_DBG = $(LFLAGS_DBG) /NOE /DLL

#CFLAGS_EXE = $(CFLAGS_EXE_DBG)
#CFLAGS_DLL = $(CFLAGS_DLL_DBG)
#LFLAGS_EXE = $(LFLAGS_EXE_DBG)
#LFLAGS_DLL = $(LFLAGS_DLL_DBG)


.SUFFIXES:	.c .obj

.c.obj:
		icc $(CFLAGS_EXE) $*.c

#

all:    gbm123.dll gbm123.lib gbmhdr.exe \
        gbmmir.lib gbmref.exe \
        gbmrect.lib gbmsub.exe \
        gbmscale.lib gbmsize.exe \
        gbmerr.lib gbmtrunc.lib gbmht.lib gbmhist.lib gbmmcut.lib gbmbpp.exe \
        gbmcpal.exe \
        gbmgamma.exe \
        gbmconv.exe \
        gbmver.exe
#

gbm123.dll:  gbm.obj    gbmpgm.obj gbmppm.obj gbmbmp.obj gbmtga.obj  \
             gbmkps.obj gbmiax.obj gbmpcx.obj gbmlbm.obj  \
             gbmvid.obj gbmgif.obj gbmxbm.obj gbmspr.obj gbmpsg.obj  \
             gbmgem.obj gbmcvp.obj gbmjpg.obj gbmhelp.obj \
             gbmmap.obj \
             gbm.def
             ilink $(LFLAGS_DLL) /OUT:$@ @<<
             $**
<<

gbm123.lib: gbm.def
            implib /NOLOGO /NOI $@ $**

gbm.obj:    gbm.c gbm.h gbmhelp.h gbmpgm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h
            icc $(CFLAGS_DLL) $*.c

# ------------
# File formats
# ------------

gbmpgm.obj:	gbmpgm.c gbm.h gbmhelp.h gbmdesc.h
		    icc $(CFLAGS_DLL) $*.c

gbmppm.obj:	gbmppm.c gbm.h gbmhelp.h gbmdesc.h
		    icc $(CFLAGS_DLL) $*.c

gbmbmp.obj:	gbmbmp.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmtga.obj:	gbmtga.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmkps.obj:	gbmkps.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmiax.obj:	gbmiax.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmpcx.obj:	gbmpcx.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmlbm.obj:	gbmlbm.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmvid.obj:	gbmvid.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmgif.obj:	gbmgif.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmxbm.obj:	gbmxbm.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmspr.obj:	gbmspr.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmpsg.obj:	gbmpsg.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmgem.obj:	gbmgem.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmcvp.obj:	gbmcvp.c gbm.h gbmhelp.h gbmdesc.h
		icc $(CFLAGS_DLL) $*.c

gbmhelp.obj:	gbmhelp.c gbm.h
		icc $(CFLAGS_DLL) $*.c

gbmmap.obj:	gbmmap.c gbm.h gbmmap.h
		icc $(CFLAGS_DLL) $*.c

# ------------
# Bitmap tools
# ------------

gbmhdr.exe:	gbmhdr.obj gbm123.lib
		ilink $(LFLAGS_EXE) /OUT:$@ /NOE setargv.obj $**

gbmhdr.obj:	gbmhdr.c gbm.h

#

gbmmir.lib:	gbmmir.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmmir.obj:	gbmmir.c

gbmref.exe:	gbmref.obj gbm123.lib gbmmir.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmref.obj:	gbmref.c gbm.h gbmmir.h

#

gbmrect.lib:	gbmrect.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmrect.obj:	gbmrect.c

gbmsub.exe:	gbmsub.obj gbm123.lib gbmrect.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmsub.obj:	gbmsub.c gbm.h gbmrect.h

#

gbmerr.lib:	gbmerr.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmerr.obj:	gbmerr.c

#

gbmscale.lib:	gbmscale.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmscale.obj:	gbmscale.c

gbmsize.exe:	gbmsize.obj gbm123.lib gbmscale.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmsize.obj:	gbmsize.c gbm.h gbmscale.h

#

gbmtrunc.lib:	gbmtrunc.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmtrunc.obj:	gbmtrunc.c

#

gbmht.lib:	gbmht.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmht.obj:	gbmht.c

#

gbmhist.lib:	gbmhist.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmhist.obj:	gbmhist.c

#

gbmmcut.lib:	gbmmcut.obj
		-del $@ 2> nul
		$(LB) $@ $**;

gbmmcut.obj:	gbmmcut.c

#

gbmbpp.exe:	gbmbpp.obj gbm123.lib gbmerr.lib gbmtrunc.lib gbmht.lib gbmhist.lib gbmmcut.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmbpp.obj:	gbmbpp.c gbm.h gbmerr.h gbmtrunc.h gbmht.h gbmhist.h gbmmcut.h

#

gbmcpal.exe:	gbmcpal.obj gbm123.lib gbmhist.lib gbmmcut.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmcpal.obj:	gbmcpal.c gbm.h gbmhist.h gbmmcut.h

#

gbmgamma.exe:	gbmgamma.obj gbm123.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmgamma.obj:	gbmgamma.c gbm.h

#

gbmconv.exe:	gbmconv.obj gbm123.lib
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmconv.obj:	gbmconv.c gbm.h

#

gbmver.exe:	gbmver.obj
		ilink $(LFLAGS_EXE) /OUT:$@ $**

gbmver.obj:	gbmver.c gbm.h

# ------------------------
# Build management targets
# ------------------------

clean:
	-@del *.obj 2> nul
	-@del *.lib 2> nul
	-@del *.exe 2> nul
	-@del *.dll 2> nul

clobber:
         -$(MAKE) clean
		 -del *.exe *.lib *.dll 2> nul
