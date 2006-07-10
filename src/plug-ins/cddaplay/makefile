#
#  cddaplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;ciscgi

!include $(TOPDIR)\config\makerules

OBJECTS  = cdda.obj readcd.obj tcpipsock.obj http.obj cddb.obj
LIBS     = $(LIBS_TCP) $(TOPDIR)\utils\utilfct.lib ciscgi\ciscgi.lib
DEF      = cdda$(LDEF_SUFFX)
RESOURCE = cdda.res

all: cddaplay.dll cddaplay.lib $(MDUMMY)

cddaplay.dll: $(OBJECTS) $(DEF) $(RESOURCE) ciscgi\ciscgi.lib
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

cddaplay.lib: $(DEF)
	$(LI) $@ cddaplay.dll

ciscgi\ciscgi.lib:
	cd ciscgi
	@$(MAKE) $(MFLAGS)
	@cd ..

clean: $(MDUMMY)
	-@del cddaplay.lib cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
	cd ciscgi
	@$(MAKE) $(MFLAGS) clean
	@cd ..
