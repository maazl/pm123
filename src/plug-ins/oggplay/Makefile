#
#  oggplay.dll makefile
#

TOPDIR  = ..\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\vrb123\include;$(TOPDIR)\xio123;$(TOPDIR)\ogg123\include
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

include $(TOPDIR)\config\makerules

OBJECTS = vcedit$(CO) oggdecoder$(CO) oggplay$(CO)
LIBS    = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\vrb123\lib\libvorbis$(LBO) \
	$(TOPDIR)\ogg123\src\libogg$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
SYSLIBS = $(LIBS_UNI)
DEF     = oggplay$(LDEF_SUFFX)

all: oggplay.dll $(MDUMMY)

oggplay.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

oggplay$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ oggplay.dll

clean:  $(MDUMMY)
	-@del oggplay$(LBI) oggplay.dll $(OBJECTS) 2> nul

oggdecoder.o:	oggdecoder.cpp ..\..\utils\cpp/interlockedptr.h  ..\..\utils\charset.h ..\..\utils\utilfct.h
oggplay.o:	oggplay.cpp ..\..\include\plugin.h ..\..\utils\fileutil.h ..\..\utils\debuglog.h vcedit.h 
vcedit.o:	vcedit.c ..\..\ogg123\include\ogg/ogg.h  vcedit.h
