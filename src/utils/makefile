#
#  utilfct.lib makefile
#

!include ..\config\makerules

all: utilfct.lib $(MDUMMY)

OBJECTS = utilfct.obj bufstream.obj fileutil.obj url.obj filefind.obj
OBJECTS = $(OBJECTS) errorstr.obj rel2abs.obj abs2rel.obj inimacro.obj
OBJECTS = $(OBJECTS) charset.obj wildcards.obj queue.obj strlcpy.obj
OBJECTS = $(OBJECTS) strlcat.obj

utilfct.lib: $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del utilfct.lib $(OBJECTS) 2> nul
