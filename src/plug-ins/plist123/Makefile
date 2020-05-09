#
#  plist123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
TARGET   = plist123.dll

include $(TOPDIR)\config\makerules

OBJECTS  = plist123$(CO) playlistreader$(CO) playlistwriter$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
DEF      = plist123$(LDEF_SUFFX)
#RESOURCE = plist123$(RES)
STDLIBS  = $(LIBS_MM)

plist123.dll: $(OBJECTS) $(LIBS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(STDLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
#	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del plist123.dll $(OBJECTS) $(RESOURCE) 2> nul

playlistreader.o:	playlistreader.cpp plist123.h ..\..\utils\cpp/container/stringmap.h ..\..\utils\charset.h ..\..\xio123\xio.h
playlistwriter.o:	playlistwriter.cpp plist123.h ..\..\utils\charset.h 
plist123.o:	plist123.cpp ..\..\include\decoder_plug.h plist123.h playlistwriter.h ..\..\xio123\xio.h  ..\..\utils\debuglog.h
