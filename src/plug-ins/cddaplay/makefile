#
#  cddaplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;
TARGET   = cddaplay.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = cdda$(CO) readcd$(CO) tcpipsock$(CO) http$(CO) cddb$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) ciscgi\ciscgi$(LBO) $(TOPDIR)\utils\cpp\cpputil$(LBO)
SYSLIBS  = $(LIBS_TCP) $(LIBS_UNI)
DEF      = cdda$(LDEF_SUFFX)
RESOURCE = cdda$(RES)

cddaplay.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE) ciscgi\ciscgi$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

ciscgi\ciscgi$(LBO):
	cd ciscgi
	@$(MAKE) $(MFLAGS)
	@cd ..

clean: $(MDUMMY)
	-@del cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
	cd ciscgi
	@$(MAKE) $(MFLAGS) clean
	@cd ..

cdda.cpp:	..\..\include\config.h ..\..\utils\inimacro.h ..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h ..\..\utils\snprintf.h readcd.h ..\..\utils\utilfct.h cdda.h cddarc.h ..\..\utils\debuglog.h
readcd.h:	cddb.h
cddb.h:	tcpipsock.h http.h
tcpipsock.h:	..\..\utils\utilfct.h
cdda.h:	cddb.h
cddb.cpp:	..\..\utils\utilfct.h readcd.h tcpipsock.h http.h cddb.h ciscgi/cgicis.h cdda.h
http.cpp:	..\..\utils\utilfct.h tcpipsock.h http.h cdda.h
readcd.cpp:	..\..\utils\utilfct.h ..\..\utils\snprintf.h readcd.h cdda.h cddarc.h ..\..\utils\debuglog.h
tcpipsock.cpp:	..\..\utils\utilfct.h tcpipsock.h cdda.h
