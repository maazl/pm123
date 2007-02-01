#
#  g72x library makefile
#

DLL     = yes
TOPDIR  = ..\..\..

!include $(TOPDIR)\config\makerules

all: g72x$(LBO) $(MDUMMY)

OBJECTS = g721$(CO) g723_16$(CO) g723_24$(CO) g723_40$(CO) g72x$(CO) 

g72x$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del g72x$(LBO) $(OBJECTS) 2> nul
