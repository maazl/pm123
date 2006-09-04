#
#  realeq.dll makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api

!include $(TOPDIR)\config\makerules

OBJECTS  = realeq$(CO)
LIBS     = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
DEF      = realeq$(LDEF_SUFFX)
RESOURCE = realeq$(RES)

all: realeq.dll realeq$(LBI) $(MDUMMY)

realeq.dll: $(OBJECTS) $(DEF) $(RESOURCE)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul

realeq$(LBI): $(DEF)
	$(LI) $@ realeq.dll

clean: $(MDUMMY)
	-@del realeq$(LBI) realeq.dll $(OBJECTS) $(RESOURCE) 2> nul
