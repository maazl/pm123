#
#  skinutil.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;$(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\pm123
EXE_IO   = yes
TARGET   = skinutil.exe

!include $(TOPDIR)\config\makerules

OBJECTS  = skinutil$(CO)
DEF      = skinutil$(LDEF_SUFFX)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\utils\utilfct$(LBO)

skinutil.exe: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del skinutil.exe $(OBJECTS) 2> nul
