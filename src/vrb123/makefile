#
#  vrb123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

DEF     = vrb$(LDEF_SUFFX)
LIBS    = lib\libvorbis$(LBO) $(TOPDIR)\ogg123\ogg123$(LBI)
OBJECTS = vrb$(CO)

all: vrb123.dll vrb123$(LBI) $(MDUMMY)

vrb123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

vrb123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ vrb123.dll

lib\libvorbis$(LBO):
	cd lib
	@$(MAKE) $(MFLAGS)
	@cd ..

clean: $(MDUMMY)
	cd lib
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	-@del vrb123.dll vrb123$(LBI) $(OBJECTS) 2> nul
