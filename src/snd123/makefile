#
#  snd123.dll makefile
#

TOPDIR  = ..
TARGET  = all2

include $(TOPDIR)\config\makerules

DEF     = snd$(LDEF_SUFFX)
LIBS    = src\sndfile$(LBO) \
	src\G72x\g72x$(LBO) \
	src\GSM610\gsm610$(LBO) \
	$(TOPDIR)\utils\utilfct$(LBO)
OBJECTS = snd$(CO)

all2: src\sndfile$(LBO) src\G72x\g72x$(LBO) src\GSM610\gsm610$(LBO) $(MDUMMY)

snd123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

snd123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ snd123.dll

src\sndfile$(LBO):
	@$(MAKE) -C src $(MFLAGS)

src\G72x\g72x$(LBO):
	@$(MAKE) -C src\G72x $(MFLAGS)

src\GSM610\gsm610$(LBO):
	@$(MAKE) -C src\GSM610 $(MFLAGS)

clean: $(MDUMMY)
	@$(MAKE) -C src $(MFLAGS) clean
	@$(MAKE) -C src\G72x $(MFLAGS) clean
	@$(MAKE) -C src\GSM610 $(MFLAGS) clean
	-@del snd123.dll snd123$(LBI) $(OBJECTS) 2> nul

depend: $(MDUMMY)
	@$(MAKE) -C src $(MFLAGS) depend
#	@$(MAKE) -C src\G72x $(MFLAGS) depend
#	@$(MAKE) -C src\GSM610 $(MFLAGS) depend
	