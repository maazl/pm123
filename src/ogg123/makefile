#
#  ogg123.dll makefile
#

TOPDIR  = ..

include $(TOPDIR)\config\makerules

DEF     = ogg$(LDEF_SUFFX)
LIBS    = src\libogg$(LBO) 
OBJECTS = ogg$(CO)

all: src\libogg$(LBO) $(MDUMMY)

ogg123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

ogg123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ ogg123.dll

src\libogg$(LBO):
	@$(MAKE) -C src $(MFLAGS)

clean: $(MDUMMY)
	@$(MAKE) -C src $(MFLAGS) clean
	-@del ogg123.dll ogg123$(LBI) $(OBJECTS) 2> nul

depend: $(MDUMMY)
	@$(MAKE) -C src $(MFLAGS) depend
