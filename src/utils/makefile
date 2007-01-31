#
#  utilfct library makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.

!include ..\config\makerules

all: utilfct$(LBO) $(MDUMMY)

OBJECTS = utilfct$(CO) bufstream$(CO) fileutil$(CO) filefind$(CO)
OBJECTS = $(OBJECTS) errorstr$(CO) rel2abs$(CO) abs2rel$(CO) inimacro$(CO)
OBJECTS = $(OBJECTS) charset$(CO) wildcards$(CO) queue$(CO) strlcpy$(CO)
OBJECTS = $(OBJECTS) strlcat$(CO) snprintf$(CO)

utilfct$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del utilfct$(LBO) $(OBJECTS) 2> nul
