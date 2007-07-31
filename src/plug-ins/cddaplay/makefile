#
#  cddaplay.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;ciscgi

!include $(TOPDIR)\config\makerules

OBJECTS  = cdda$(CO) readcd$(CO) tcpipsock$(CO) http$(CO) cddb$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) ciscgi\ciscgi$(LBO) $(TOPDIR)\utils\cpp\cpputil$(LBO) $(LIBS_TCP) $(LIBS_UNI)
DEF      = cdda$(LDEF_SUFFX)
RESOURCE = cdda$(RES)

all: cddaplay.dll cddaplay$(LBI) $(MDUMMY)

cddaplay.dll: $(OBJECTS) $(DEF) $(RESOURCE) ciscgi\ciscgi$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

cddaplay$(LBI): $(DEF)
	$(LI) $@ cddaplay.dll

ciscgi\ciscgi$(LBO):
	cd ciscgi
	@$(MAKE) $(MFLAGS)
	@cd ..

cddb$(CO): cddb.cpp ciscgi\cgicis.h
readcd$(CO): readcd.cpp readcd.h

clean: $(MDUMMY)
	-@del cddaplay$(LBI) cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
	cd ciscgi
	@$(MAKE) $(MFLAGS) clean
	@cd ..
