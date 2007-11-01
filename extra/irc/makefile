#
#  querysong.exe makefile
#

TOPDIR = ..\..
TARGET = querysong.exe

!include $(TOPDIR)\src\config\makerules

OBJECTS  = querysong$(CO)
DEF      = querysong$(LDEF_SUFFX)

querysong.exe: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

clean: $(MDUMMY)
	-@del querysong.exe $(OBJECTS) 2> nul
