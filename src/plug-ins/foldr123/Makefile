#
#  foldr123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
TARGET   = foldr123.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = foldr123$(CO) dirscan$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO)
DEF      = foldr123$(LDEF_SUFFX)
#RESOURCE = foldr123$(RES)
STDLIBS  = $(LIBS_MM)

foldr123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
#	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del foldr123.dll $(OBJECTS) $(RESOURCE) 2> nul

dirscan.h:	..\..\include\decoder_plug.h ..\..\utils\cpp\xstring.h ..\..\utils\cpp\container\vector.h
..\..\include\decoder_plug.h:	..\..\include\format.h ..\..\include\output_plug.h
..\..\include\format.h:	..\..\include\plugin.h ..\..\utils\cpp\xstring.h
..\..\include\plugin.h:	..\..\include\config.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
..\..\utils\cpp\xstring.h:	..\..\utils\cpp\smartptr.h
..\..\utils\cpp\smartptr.h:	..\..\include\config.h ..\..\utils\cpp\cpputil.h ..\..\utils\cpp\mutex.h ..\..\utils\debuglog.h
..\..\utils\cpp\mutex.h:	..\..\include\config.h ..\..\utils\interlocked.h ..\..\utils\debuglog.h
..\..\utils\interlocked.h:	..\..\include\config.h
..\..\utils\debuglog.h:	..\..\utils\errorstr.h
..\..\include\output_plug.h:	..\..\include\format.h
foldr123.cpp:	..\..\include\format.h ..\..\include\decoder_plug.h ..\..\include\plugin.h foldr123.h dirscan.h ..\..\utils\errorstr.h ..\..\utils\debuglog.h
..\..\utils\cpp\container\vector.h:	..\..\utils\cpp\mutex.h ..\..\utils\cpp\smartptr.h
..\..\utils\cpp\algorithm.h:	..\..\utils\cpp\container\vector.h
dirscan.cpp:	dirscan.h ..\..\utils\eautils.h ..\..\utils\strutils.h ..\..\xio123\xio.h ..\..\utils\cpp\smartptr.h ..\..\utils\cpp\algorithm.h
..\..\xio123\xio.h:	..\..\include\config.h
