#
#  gbm123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

OBJECTS = gbm$(CO) gbmpgm$(CO) gbmppm$(CO) gbmbmp$(CO) gbmtga$(CO) gbmkps$(CO)
OBJECTS = $(OBJECTS) gbmiax$(CO) gbmpcx$(CO) gbmlbm$(CO) gbmvid$(CO) gbmgif$(CO)
OBJECTS = $(OBJECTS) gbmxbm$(CO) gbmspr$(CO) gbmpsg$(CO) gbmgem$(CO) gbmcvp$(CO)
OBJECTS = $(OBJECTS) gbmjpg$(CO) gbmhelp$(CO) gbmmap$(CO) gbmrect$(CO) gbmpbm$(CO)
OBJECTS = $(OBJECTS) gbmpnm$(CO)  
DEF     = gbm$(LDEF_SUFFX)

all: gbm123.dll gbm123$(LBI) $(MDUMMY)

gbm123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

gbm123$(LBI): $(DEF)
	$(LI) $@ gbm123.dll

gbm$(CO):     gbm.c gbm.h gbmhelp.h gbmpgm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h
gbmpgm$(CO):  gbmpgm.c gbm.h gbmhelp.h gbmdesc.h
gbmppm$(CO):  gbmppm.c gbm.h gbmhelp.h gbmdesc.h
gbmbmp$(CO):  gbmbmp.c gbm.h gbmhelp.h gbmdesc.h
gbmtga$(CO):  gbmtga.c gbm.h gbmhelp.h gbmdesc.h
gbmkps$(CO):  gbmkps.c gbm.h gbmhelp.h gbmdesc.h
gbmiax$(CO):  gbmiax.c gbm.h gbmhelp.h gbmdesc.h
gbmpcx$(CO):  gbmpcx.c gbm.h gbmhelp.h gbmdesc.h
gbmlbm$(CO):  gbmlbm.c gbm.h gbmhelp.h gbmdesc.h
gbmvid$(CO):  gbmvid.c gbm.h gbmhelp.h gbmdesc.h
gbmgif$(CO):  gbmgif.c gbm.h gbmhelp.h gbmdesc.h
gbmxbm$(CO):  gbmxbm.c gbm.h gbmhelp.h gbmdesc.h
gbmspr$(CO):  gbmspr.c gbm.h gbmhelp.h gbmdesc.h
gbmpsg$(CO):  gbmpsg.c gbm.h gbmhelp.h gbmdesc.h
gbmgem$(CO):  gbmgem.c gbm.h gbmhelp.h gbmdesc.h
gbmcvp$(CO):  gbmcvp.c gbm.h gbmhelp.h gbmdesc.h
gbmhelp$(CO): gbmhelp.c gbm.h
gbmmap$(CO):  gbmmap.c gbm.h gbmmap.h

clean: $(MDUMMY)
	-@del gbm123$(LBI) gbm123.dll $(OBJECTS) 2> nul
