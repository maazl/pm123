#
#  utilfct library makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: utilfct$(LBO) $(MDUMMY)

OBJECTS = utilfct$(CO) bufstream$(CO) fileutil$(CO) filefind$(CO)
OBJECTS = $(OBJECTS) errorstr$(CO) rel2abs$(CO) abs2rel$(CO) inimacro$(CO)
OBJECTS = $(OBJECTS) charset$(CO) wildcards$(CO) queue$(CO) strlcpy$(CO) strchomp$(CO)
OBJECTS = $(OBJECTS) strlcat$(CO) snprintf$(CO) strnchr$(CO) memcmpcpy$(CO)
OBJECTS = $(OBJECTS) vdelegate$(CO) debuglog$(CO)
STDLIBS = $(LIBS_UNI)

utilfct$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

charset.exe:	charset.obj
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	
clean: $(MDUMMY)
	-@del utilfct$(LBO) $(OBJECTS) 2> nul

abs2rel.c:	abs2rel.h
bufstream.c:	bufstream.h
charset.c:	charset.h strutils.h debuglog.h utilfct.h
utilfct.h:	..\include\config.h .\rel2abs.h .\abs2rel.h .\bufstream.h .\charset.h .\wildcards.h .\filefind.h .\fileutil.h .\errorstr.h .\inimacro.h .\queue.h .\minmax.h .\strutils.h
errorstr.c:	strutils.h snprintf.h
fileutil.c:	fileutil.h strutils.h minmax.h
inimacro.c:	utilfct.h
memcmpcpy.c:	strutils.h
queue.c:	queue.h
rel2abs.c:	rel2abs.h
strnchr.c:	strutils.h
utilfct.c:	utilfct.h debuglog.h
vdelegate.c:	vdelegate.h
vdelegate.h:	..\include\format.h
wildcards.c:	wildcards.h
debuglog.h:	.\errorstr.h
fileutil.h:	..\include\config.h
pcmfloat2int.c:	pcmconvert.h utilfct.h debuglog.h
pcmconvert.h:	..\include\format.h
pcmfloat2int_merge.c:	pcmconvert.h utilfct.h debuglog.h
pcmint2float.c:	pcmconvert.h .\debuglog.h
pcmint2float_extract.c:	pcmconvert.h .\debuglog.h
debuglog.c:	debuglog.h snprintf.h
strchomp.c:	strutils.h
