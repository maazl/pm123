#
#  snd123.dll makefile
#

DLL     = yes
TOPDIR  = ..

!include $(TOPDIR)\config\makerules

DEF     = snd$(LDEF_SUFFX)
LIBS    = src\sndfile$(LBO) 
LIBS    = $(LIBS) src\G72x\g72x$(LBO)
LIBS    = $(LIBS) src\GSM610\gsm610$(LBO)
LIBS    = $(LIBS) $(TOPDIR)\utils\utilfct$(LBO)
OBJECTS = snd$(CO)

all: snd123.dll snd123$(LBI) $(MDUMMY)

snd123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX) $(OBJECTS) $(LIBS) $(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

snd123$(LBI): $(DEF)
	$(LI) $@ snd123.dll

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