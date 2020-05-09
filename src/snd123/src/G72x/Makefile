#
#  g72x library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: g72x$(LBO) $(MDUMMY)

OBJECTS = g721$(CO) g723_16$(CO) g723_24$(CO) g723_40$(CO) g72x$(CO) 

g72x$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del g72x$(LBO) $(OBJECTS) 2> nul

g721.c:		g72x.h g72x_priv.h
g723_16.c:	g72x.h g72x_priv.h
g723_24.c:	g72x.h g72x_priv.h
g723_40.c:	g72x.h g72x_priv.h
g72x.c:		g72x.h g72x_priv.h
g72x_test.c:	g72x.h g72x_priv.h
