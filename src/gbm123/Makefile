#
#  gbm123.dll makefile
#

TOPDIR  = ..
TARGET  = all2
INCDIR   = $(TOPDIR)\include
#CFLAGS_USR = -DENABLE_PNG

include $(TOPDIR)\config\makerules

OBJECTS = gbm$(CO) gbmpgm$(CO) gbmppm$(CO) gbmbmp$(CO) gbmtga$(CO) gbmkps$(CO) \
	gbmiax$(CO) gbmpcx$(CO) gbmlbm$(CO) gbmvid$(CO) gbmgif$(CO) \
	gbmxbm$(CO) gbmspr$(CO) gbmpsg$(CO) gbmgem$(CO) gbmcvp$(CO) \
	gbmjpg$(CO) gbmhelp$(CO) gbmmap$(CO) gbmrect$(CO) gbmpbm$(CO) \
	gbmpnm$(CO)
# gbmpng$(CO)  
DEF     = gbm$(LDEF_SUFFX)

all2: libgbm$(LBO) $(MDUMMY)

gbm123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

gbm123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ gbm123.dll

libgbm$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

OBJECTS_TOOL = gbmtool$(CO) gbmtos2$(CO)

libgbmtool$(LBO): $(OBJECTS_TOOL)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS_TOOL) $(LBENDL)

gbmconv.exe: gbmconv$(CO) $(LIBS) libgbm$(LBO) libgbmtool$(LBO)
	$(CL) $(LFLAGS) $(LFLAGS_IO) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)gbmconv$(CO)$(LOBJ_SUFFX) $(LLIB_PREFX)libgbmtool$(LBO) libgbm$(LBO) $(LIBS)$(LLIB_SUFFX)
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del gbm123$(LBI) gbm123.dll gbmconv.exe $(OBJECTS) 2> nul

gbm.c:		..\include\sys\io.h gbm.h gbmhelp.h gbmpbm.h gbmpgm.h gbmpnm.h gbmppm.h gbmpng.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h
gbmbmp.c:	gbm.h gbmhelp.h gbmdesc.h
gbmbpp.c:	gbm.h gbmerr.h gbmtrunc.h gbmht.h gbmhist.h gbmmcut.h gbmtool.h
gbmconv.c:	gbm.h gbmtool.h
gbmcpal.c:	gbm.h gbmhist.h gbmmcut.h gbmtool.h
gbmcvp.c:	gbm.h gbmhelp.h gbmdesc.h
gbmerr.c:	gbm.h
gbmgamma.c:	gbm.h gbmtool.h
gbmgem.c:	gbm.h gbmhelp.h gbmdesc.h
gbmgif.c:	gbm.h gbmhelp.h gbmdesc.h
gbmhdr.c:	gbm.h gbmtool.h
gbmhelp.c:	gbm.h
gbmhist.c:	gbm.h
gbmht.c:	gbm.h
gbmiax.c:	gbm.h gbmhelp.h gbmdesc.h
gbmjpg.c:	gbm.h gbmhelp.h gbmdesc.h
gbmkps.c:	gbm.h gbmhelp.h gbmdesc.h
gbmlbm.c:	gbm.h gbmhelp.h gbmdesc.h
gbmmap.c:	gbm.h gbmmap.h
gbmmcut.c:	gbm.h
gbmmir.c:	gbm.h
gbmpbm.c:	gbm.h gbmhelp.h gbmdesc.h
gbmpcx.c:	gbm.h gbmhelp.h gbmdesc.h
gbmpgm.c:	gbm.h gbmhelp.h gbmdesc.h
gbmpng.c:	gbm.h gbmhelp.h gbmdesc.h gbmmap.h
gbmpnm.c:	gbm.h gbmhelp.h gbmdesc.h gbmpbm.h gbmpgm.h gbmppm.h
gbmppm.c:	gbm.h gbmhelp.h gbmdesc.h
gbmpsg.c:	gbm.h gbmhelp.h gbmdesc.h
gbmrect.c:	gbm.h
gbmref.c:	gbm.h gbmmir.h gbmtool.h
gbmscale.c:	gbm.h
gbmsize.c:	gbm.h gbmscale.h gbmtool.h
gbmspr.c:	gbm.h gbmhelp.h gbmdesc.h
gbmsub.c:	gbm.h gbmrect.h gbmtool.h
gbmtga.c:	gbm.h gbmhelp.h gbmdesc.h
gbmtif.c:	gbm.h gbmhelp.h gbmdesc.h gbmmap.h
gbmtool.c:	gbm.h gbmtool.h
gbmtos2.c:	gbm.h gbmtool.h
gbmtrunc.c:	gbm.h
gbmver.c:	gbm.h gbmtool.h
gbmvid.c:	gbm.h gbmhelp.h gbmdesc.h
gbmxbm.c:	gbm.h gbmhelp.h gbmdesc.h
gbmspr.o:	gbmspr.c gbmhelp.h 
gbm.o:		gbm.c gbm.h  gbmpbm.h gbmpnm.h gbmpng.h gbmtga.h gbmiax.h gbmtif.h gbmvid.h gbmxbm.h gbmpsg.h gbmcvp.h
gbmsize.o:	gbmsize.c gbmscale.h 
gbmver.o:	gbmver.c gbmtool.h
gbmvid.o:	gbmvid.c gbmhelp.h 
gbmbmp.o:	gbmbmp.c gbmhelp.h 
gbmxbm.o:	gbmxbm.c gbmhelp.h 
gbmbpp.o:	gbmbpp.c gbmerr.h gbmht.h gbmmcut.h 
gbmconv.o:	gbmconv.c gbmtool.h
gbmcpal.o:	gbmcpal.c gbmhist.h gbmtool.h
gbmcvp.o:	gbmcvp.c gbmhelp.h 
gbmerr.o:	gbmerr.c 
gbmgamma.o:	gbmgamma.c gbmtool.h
gbmgem.o:	gbmgem.c gbmhelp.h 
gbmgif.o:	gbmgif.c gbmhelp.h 
gbmhdr.o:	gbmhdr.c gbmtool.h
gbmhelp.o:	gbmhelp.c 
gbmhist.o:	gbmhist.c 
gbmht.o:	gbmht.c 
gbmiax.o:	gbmiax.c gbmhelp.h 
gbmjpg.o:	gbmjpg.c gbmhelp.h 
gbmkps.o:	gbmkps.c gbmhelp.h 
gbmlbm.o:	gbmlbm.c gbmhelp.h 
gbmmap.o:	gbmmap.c gbmmap.h
gbmmcut.o:	gbmmcut.c 
gbmmir.o:	gbmmir.c 
gbmpbm.o:	gbmpbm.c gbmhelp.h 
gbmpcx.o:	gbmpcx.c gbmhelp.h 
gbmpgm.o:	gbmpgm.c gbmhelp.h 
gbmpng.o:	gbmpng.c gbmhelp.h gbmmap.h
gbmpnm.o:	gbmpnm.c gbmhelp.h gbmpbm.h gbmppm.h
gbmppm.o:	gbmppm.c gbmhelp.h 
gbmpsg.o:	gbmpsg.c gbmhelp.h 
gbmrect.o:	gbmrect.c 
gbmref.o:	gbmref.c gbmmir.h 
gbmscale.o:	gbmscale.c 
gbmsub.o:	gbmsub.c gbmrect.h 
gbmtga.o:	gbmtga.c gbmhelp.h 
gbmtif.o:	gbmtif.c gbmhelp.h gbmmap.h
gbmtool.o:	gbmtool.c gbmtool.h
gbmtos2.o:	gbmtos2.c gbmtool.h
gbmtrunc.o:	gbmtrunc.c 
