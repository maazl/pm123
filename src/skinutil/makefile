#
#  skinutil.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\pm123
TARGET   = skinutil.exe

!include $(TOPDIR)\config\makerules

OBJECTS  = skinutil$(CO)
DEF      = skinutil$(LDEF_SUFFX)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\utils\utilfct$(LBO)

skinutil.exe: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_IO) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del skinutil.exe $(OBJECTS) 2> nul

skinutil.c:	..\gbm123\gbm.h ..\gbm123\gbmrect.h ..\utils\utilfct.h ..\pm123\skin.h ..\pm123\copyright.h
