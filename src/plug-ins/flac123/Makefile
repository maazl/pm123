#
#  flac123.dll makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\xio123
INCDIR   = $(INCDIR);$(TOPDIR)\libflac\include
CFLAGS_USR = -DPLUGIN_INTERFACE_LEVEL=3

!include $(TOPDIR)\config\makerules

OBJECTS  = decoder$(CO) flac123$(CO)
LIBS     = $(TOPDIR)\utils\cpp\cpputil_plugin$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
LIBS     = $(LIBS) $(TOPDIR)\libflac\src\libFLAC\libFLAC$(LBO) $(TOPDIR)\libflac\src\libFLAC++\libFLAC++$(LBO)
#LIBS     = $(LIBS) $(TOPDIR)\ogg123\src\libogg$(LBO) $(TOPDIR)\xio123\xio123$(LBI)
LIBS     = $(LIBS) $(TOPDIR)\xio123\xio123$(LBI)
SYSLIBS  = $(LIBS_UNI)
DEF      = flac123$(LDEF_SUFFX)

all: flac123.dll $(MDUMMY)

flac123.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

flac123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ flac123.dll

clean:  $(MDUMMY)
	-@del flac123$(LBI) flac123.dll $(OBJECTS) 2> nul

