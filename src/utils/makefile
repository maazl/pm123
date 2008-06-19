#
#  utilfct library makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include

!include $(TOPDIR)\config\makerules

all: utilfct$(LBO) $(MDUMMY)

OBJECTS = utilfct$(CO) bufstream$(CO) fileutil$(CO) filefind$(CO)
OBJECTS = $(OBJECTS) errorstr$(CO) rel2abs$(CO) abs2rel$(CO) inimacro$(CO)
OBJECTS = $(OBJECTS) charset$(CO) wildcards$(CO) queue$(CO) strlcpy$(CO)
OBJECTS = $(OBJECTS) strlcat$(CO) snprintf$(CO) eautils$(CO)

utilfct$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean:  $(MDUMMY)
	-@del utilfct$(LBO) $(OBJECTS) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

abs2rel.c:	abs2rel.h
bufstream.c:	bufstream.h
charset.c:	charset.h strutils.h debuglog.h
debuglog.h:	snprintf.h
errorstr.c:	strutils.h
fileutil.c:	fileutil.h strutils.h minmax.h
inimacro.c:	utilfct.h
utilfct.h:	..\include\config.h rel2abs.h abs2rel.h bufstream.h charset.h wildcards.h filefind.h fileutil.h errorstr.h inimacro.h queue.h minmax.h strutils.h
queue.c:	queue.h
rel2abs.c:	rel2abs.h
utilfct.c:	utilfct.h
wildcards.c:	wildcards.h
eautils.c:	debuglog.h
