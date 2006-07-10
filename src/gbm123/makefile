#
#  gbm123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

OBJECTS = gbm.obj gbmpgm.obj gbmppm.obj gbmbmp.obj gbmtga.obj gbmkps.obj
OBJECTS = $(OBJECTS) gbmiax.obj gbmpcx.obj gbmlbm.obj gbmvid.obj gbmgif.obj
OBJECTS = $(OBJECTS) gbmxbm.obj gbmspr.obj gbmpsg.obj gbmgem.obj gbmcvp.obj
OBJECTS = $(OBJECTS) gbmjpg.obj gbmhelp.obj gbmmap.obj gbmrect.obj gbmpbm.obj
OBJECTS = $(OBJECTS) gbmpnm.obj  
DEF     = gbm$(LDEF_SUFFX)

all: gbm123.dll gbm123.lib $(MDUMMY)

gbm123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

gbm123.lib: $(DEF)
	$(LI) $@ gbm123.dll

gbm.obj:     gbm.c gbm.h gbmhelp.h gbmpgm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h
gbmpgm.obj:  gbmpgm.c gbm.h gbmhelp.h gbmdesc.h
gbmppm.obj:  gbmppm.c gbm.h gbmhelp.h gbmdesc.h
gbmbmp.obj:  gbmbmp.c gbm.h gbmhelp.h gbmdesc.h
gbmtga.obj:  gbmtga.c gbm.h gbmhelp.h gbmdesc.h
gbmkps.obj:  gbmkps.c gbm.h gbmhelp.h gbmdesc.h
gbmiax.obj:  gbmiax.c gbm.h gbmhelp.h gbmdesc.h
gbmpcx.obj:  gbmpcx.c gbm.h gbmhelp.h gbmdesc.h
gbmlbm.obj:  gbmlbm.c gbm.h gbmhelp.h gbmdesc.h
gbmvid.obj:  gbmvid.c gbm.h gbmhelp.h gbmdesc.h
gbmgif.obj:  gbmgif.c gbm.h gbmhelp.h gbmdesc.h
gbmxbm.obj:  gbmxbm.c gbm.h gbmhelp.h gbmdesc.h
gbmspr.obj:  gbmspr.c gbm.h gbmhelp.h gbmdesc.h
gbmpsg.obj:  gbmpsg.c gbm.h gbmhelp.h gbmdesc.h
gbmgem.obj:  gbmgem.c gbm.h gbmhelp.h gbmdesc.h
gbmcvp.obj:  gbmcvp.c gbm.h gbmhelp.h gbmdesc.h
gbmhelp.obj: gbmhelp.c gbm.h
gbmmap.obj:  gbmmap.c gbm.h gbmmap.h

clean: $(MDUMMY)
	-@del gbm123.lib gbm123.dll $(OBJECTS) 2> nul
