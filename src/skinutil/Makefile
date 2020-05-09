#
#  skinutil.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\pm123
TARGET   = skinutil.exe

include $(TOPDIR)\config\makerules

OBJECTS  = skinutil$(CO)
DEF      = skinutil$(LDEF_SUFFX)
LIBS     = $(TOPDIR)\gbm123\libgbm$(LBO) $(TOPDIR)\utils\utilfct$(LBO)

skinutil.exe: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_IO) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

clean:  $(MDUMMY)
	-@del skinutil.exe $(OBJECTS) 2> nul

skinutil.o:	skinutil.c ..\gbm123\gbmrect.h ..\pm123\gui\skin.h 
