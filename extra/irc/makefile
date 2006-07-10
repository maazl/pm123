#
#  querysong.exe makefile
#

!include ..\..\src\config\makerules

all: querysong.exe $(MDUMMY)

OBJECTS  = querysong.obj
DEF      = querysong$(LDEF_SUFFX)

querysong.exe: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

clean: $(MDUMMY)
	-@del querysong.exe $(OBJECTS) 2> nul
