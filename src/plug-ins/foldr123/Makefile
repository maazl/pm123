#
#  foldr123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
TARGET   = foldr123.dll

include $(TOPDIR)\config\makerules

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

dirscan.o:	dirscan.cpp ..\..\utils\eautils.h ..\..\xio123\xio.h ..\..\utils\cpp/algorithm.h 
foldr123.o:	foldr123.cpp ..\..\include\decoder_plug.h foldr123.h ..\..\utils\errorstr.h 
