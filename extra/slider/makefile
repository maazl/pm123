#
#  slider.exe makefile
#

EXE_PM   = yes
TARGET   = slider.exe

!include ..\..\src\config\makerules

OBJECTS  = slider$(CO)
DEF      = slider$(LDEF_SUFFX)
RESOURCE = slider$(RES)

slider.exe: $(OBJECTS) $(RESOURCE) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

clean: $(MDUMMY)
	-@del slider.exe $(OBJECTS) $(RESOURCE) 2> nul
