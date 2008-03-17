#
#  analyzer.dll makefile
#

#DEBUG=yes
#DEBUG_ALLOC=yes

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include;$(TOPDIR)\fft123\api
TARGET   = analyzer.dll

!include $(TOPDIR)\config\makerules

OBJECTS  = colormap$(CO) specana$(CO) analyzer$(CO)
MYLIBS   = $(TOPDIR)\utils\utilfct$(LBO) $(TOPDIR)\fft123\fft123$(LBI)
LIBS     = $(LIBS_MM) $(MYLIBS)
DEF      = analyzer$(LDEF_SUFFX)
RESOURCE = analyzer$(RES)

analyzer.dll: $(OBJECTS) $(DEF) $(RESOURCE) $(MYLIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(RC) $(RFLAGS) $(RESOURCE) $@ > nul
	$(POSTLINKCMD)

clean: $(MDUMMY)
	-@del analyzer.dll $(OBJECTS) $(RESOURCE) 2> nul

specana$(CO) : specana.h
analyzer$(CO) : analyzer.h colormap.h specana.h
colormap$(CO) : colormap.h analyzer.h