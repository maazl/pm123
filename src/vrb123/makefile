#
#  vrb123.dll makefile
#

TOPDIR  = ..

include $(TOPDIR)\config\makerules

DEF     = vrb$(LDEF_SUFFX)
LIBS    = lib\libvorbis$(LBO) $(TOPDIR)\ogg123\src\libogg$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
OBJECTS = vrb$(CO)

all: lib\libvorbis$(LBO) $(MDUMMY)

vrb123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

vrb123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ vrb123.dll

lib\libvorbis$(LBO):
	@$(MAKE) -C lib $(MFLAGS)

clean: $(MDUMMY)
	@$(MAKE) -C lib $(MFLAGS) clean
	-@del vrb123.dll vrb123$(LBI) $(OBJECTS) 2> nul

depend: $(MDUMMY)
	@$(MAKE) -C lib $(MFLAGS) depend
