#
#  analyzer.dll makefile
#

#DEBUG=yes
#DEBUG_ALLOC=yes

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api

!include $(TOPDIR)\config\makerules

OBJECTS  = colormap$(CO) specana$(CO) analyzer$(CO)
MYLIBS   = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
LIBS     = $(LIBS_MM) $(MYLIBS)
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer$(RES)

all: analyzer.dll analyzer$(LBI) $(MDUMMY)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(MYLIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

analyzer$(LBI): $(DEF)
	$(LI) $@ analyzer.dll

clean: $(MDUMMY)
	-@del analyzer$(LBI) analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul

specana$(CO) : specana.h
analyzer$(CO) : analyzer.h colormap.h specana.h
colormap$(CO) : colormap.h analyzer.h