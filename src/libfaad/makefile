#
#  libfaad.dll makefile
#

TOPDIR  = ..
TARGET  = all2

include $(TOPDIR)\config\makerules

DEF     = libfaad$(LDEF_SUFFX)
LIBS    = libfaad\libfaad$(LBO)
OBJECTS =

all2: libfaad\libfaad$(LBO) common\mp4ff\mp4ff$(LBO) $(MDUMMY)

libfaad.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

libfaad$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ libfaad.dll

libfaad\libfaad$(LBO):
	$(MAKE) -C libfaad $(MFLAGS)

common\mp4ff\mp4ff$(LBO):
	$(MAKE) -C common\mp4ff $(MFLAGS)

clean: $(MDUMMY)
	$(MAKE) -C libfaad $(MFLAGS) clean
	$(MAKE) -C common\mp4ff $(MFLAGS) clean
	-@del libfaad.dll libfaad$(LBI) $(OBJECTS) 2> nul

depend: $(MDUMMY)
	$(MAKE) -C libfaad $(MFLAGS) depend
	$(MAKE) -C common\mp4ff $(MFLAGS) depend
	