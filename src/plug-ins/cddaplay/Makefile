#
#  cddaplay.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;
TARGET   = cddaplay.dll

include $(TOPDIR)\config\makerules

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
	@$(MAKE) -C ciscgi $(MFLAGS)

clean: $(MDUMMY)
	-@del cddaplay.dll $(OBJECTS) $(RESOURCE) 2> nul
	@$(MAKE) -C ciscgi $(MFLAGS) clean

cdda.o:		cdda.cpp ..\..\include\format.h ..\..\include\plugin.h readcd.h  ..\..\utils\inimacro.h cdda.h ..\..\utils\debuglog.h
cddb.o:		cddb.cpp readcd.h  http.h ciscgi\cgicis.h
http.o:		http.cpp tcpipsock.h  cdda.h
readcd.o:	readcd.cpp ..\..\utils\snprintf.h cdda.h  ..\..\utils\debuglog.h
tcpipsock.o:	tcpipsock.cpp tcpipsock.h 
ciscgi\cgifldld.o:	ciscgi\cgifldld.c 
ciscgi\cgiupper.o:	ciscgi\cgiupper.c 
ciscgi\urldcode.o:	ciscgi\urldcode.c 
ciscgi\urlncode.o:	ciscgi\urlncode.c 
ciscgi\urlvalue.o:	ciscgi\urlvalue.c 
