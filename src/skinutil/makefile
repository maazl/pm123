#
#  skinutil.exe makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\gbm123;$(TOPDIR)\pm123
EXE_IO   = yes

!include $(TOPDIR)\config\makerules

OBJECTS  = skinutil$(CO)
DEF      = skinutil$(LDEF_SUFFX)
LIBS     = $(TOPDIR)\gbm123\gbm123$(LBI) $(TOPDIR)\utils\utilfct$(LBO)

all: skinutil.exe $(MDUMMY)

skinutil.exe: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

clean: $(MDUMMY)
	-@del skinutil.exe $(OBJECTS) 2> nul
