#
#  skinutil.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\pm123
EXE_IO   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = skinutil$(CO)
DEF      = skinutil$(LDEF_SUFFX)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\utils\utilfct$(LBO)

all: skinutil.exe $(MDUMMY)

skinutil.exe: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)

clean:  $(MDUMMY)
	-@del skinutil.exe $(OBJECTS) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

skinutil.c:	..\gbm123\gbm.h ..\gbm123\gbmrect.h ..\utils\utilfct.h ..\pm123\skin.h ..\pm123\copyright.h
