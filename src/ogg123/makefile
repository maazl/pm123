#
#  ogg123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

DEF     = ogg$(LDEF_SUFFX)
LIBS    = src\libogg$(LBO) 
OBJECTS = ogg$(CO)

all: ogg123.dll ogg123$(LBI) $(MDUMMY)

ogg123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

ogg123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ ogg123.dll

src\libogg$(LBO):
	cd src
	@$(MAKE) $(MFLAGS)
	@cd ..

clean: $(MDUMMY)
	cd src
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	-@del ogg123.dll ogg123$(LBI) $(OBJECTS) 2> nul
