#
#  snd123.dll makefile
#

TOPDIR  = ..
TARGET  = all2

!include $(TOPDIR)\config\makerules

DEF     = snd$(LDEF_SUFFX)
LIBS    = src\sndfile$(LBO) 
LIBS    = $(LIBS) src\G72x\g72x$(LBO)
LIBS    = $(LIBS) src\GSM610\gsm610$(LBO)
LIBS    = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO)
OBJECTS = snd$(CO)

all2: snd123.dll snd123$(LBI) $(MDUMMY)

snd123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

snd123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ snd123.dll

src\sndfile$(LBO):
	cd src
	@$(MAKE) $(MFLAGS)
	@cd ..

src\G72x\g72x$(LBO):
	cd src\G72x
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\GSM610\gsm610$(LBO):
	cd src\GSM610
	@$(MAKE) $(MFLAGS)
	@cd ..\..

clean: $(MDUMMY)
	cd src
	@$(MAKE) $(MFLAGS) clean
	@cd ..
	cd src\G72x
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\GSM610
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	-@del snd123.dll snd123$(LBI) $(OBJECTS) 2> nul