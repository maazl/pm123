#
#  http123.dll makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR)\utils

!include $(TOPDIR)\config\makerules

OBJECTS = httpget$(CO) sockfile$(CO)
LIBS    = $(LIBS_TCP) $(TOPDIR)\utils\utilfct$(LBO)
DEF     = http$(LDEF_SUFFX)

all: http123.dll http123$(LBI) $(MDUMMY)

http123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

http123$(LBI): $(DEF)
	$(LI) $@ http123.dll

sockfile$(CO): sockfile.c httpget.h sockfile.h
httpget$(CO):  httpget.c  httpget.h

clean: $(MDUMMY)
	-@del http123$(LBI) http123.dll $(OBJECTS) 2> nul
