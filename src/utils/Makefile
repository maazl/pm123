#
#  utilfct library makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: utilfct$(LBO) $(MDUMMY)

OBJECTS = utilfct$(CO) utilfct_listbox$(CO) utilfct_menu$(CO) utilfct_notebook$(CO)
OBJECTS = $(OBJECTS) utilfct_dlg$(CO) utilfct_pm_misc$(CO) utilfct_thread$(CO) utilfct_slider$(CO)
OBJECTS = $(OBJECTS) bufstream$(CO) fileutil$(CO) filefind$(CO) fileheader$(CO)
OBJECTS = $(OBJECTS) errorstr$(CO) rel2abs$(CO) abs2rel$(CO) inimacro$(CO)
OBJECTS = $(OBJECTS) charset$(CO) wildcards$(CO) queue$(CO) strlcpy$(CO) strchomp$(CO)
OBJECTS = $(OBJECTS) strlcat$(CO) snprintf$(CO) eautils$(CO) strnchr$(CO) strnlen$(CO)
OBJECTS = $(OBJECTS) strutils$(CO)
OBJECTS = $(OBJECTS) memcmpcpy$(CO) vdelegate$(CO) interlocked$(CO) debuglog$(CO)
STDLIBS = $(LIBS_UNI)

utilfct$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

charset.exe:	charset.obj
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	
clean: $(MDUMMY)
	-@del utilfct$(LBO) $(OBJECTS) 2> nul

bufstream.c:	bufstream.h
charset.c:	charset.h strutils.h debuglog.h utilfct.h
utilfct.h:	..\include\config.h .\errorstr.h .\minmax.h .\strutils.h
errorstr.c:	strutils.h snprintf.h
fileutil.c:	fileutil.h strutils.h minmax.h
inimacro.cpp:	inimacro.h fileutil.h utilfct.h
memcmpcpy.c:	strutils.h
queue.c:	queue.h
strnchr.c:	strutils.h
utilfct.c:	utilfct.h debuglog.h
vdelegate.c:	vdelegate.h
vdelegate.h:	..\include\format.h
wildcards.c:	wildcards.h .\debuglog.h
debuglog.h:	.\errorstr.h
fileutil.h:	..\include\config.h
pcmfloat2int.c:	pcmconvert.h utilfct.h debuglog.h
pcmconvert.h:	..\include\format.h
pcmfloat2int_merge.c:	pcmconvert.h utilfct.h debuglog.h
pcmint2float.c:	pcmconvert.h .\debuglog.h
pcmint2float_extract.c:	pcmconvert.h .\debuglog.h
debuglog.c:	debuglog.h snprintf.h
strchomp.c:	strutils.h
eautils.c:	debuglog.h
inimacro.h:	..\include\config.h
interlocked.cpp:	interlocked.h
interlocked.h:	..\include\config.h
fileheader.c:	debuglog.h
strnlen.c:	strutils.h
utilfct_listbox.c:	utilfct.h debuglog.h
utilfct_menu.c:	utilfct.h debuglog.h
utilfct_notebook.c:	utilfct.h debuglog.h
utilfct_pm_misc.c:	utilfct.h debuglog.h
utilfct_thread.c:	utilfct.h debuglog.h
utilfct_slider.c:	utilfct.h debuglog.h
..\include\config.h:	..\include\config_gcc.h ..\include\config_wcc.h ..\include\config_icc_36.h ..\include\config_icc_30.h
strutils.c:	strutils.h
..\include\format.h:	..\include\plugin.h
..\include\plugin.h:	..\include\config.h .\cpp/xstring.h
.\cpp/xstring.h:	.\cpp/smartptr.h
.\cpp/smartptr.h:	..\include\config.h .\cpp/cpputil.h .\cpp/mutex.h .\debuglog.h
.\cpp/mutex.h:	..\include\config.h .\interlocked.h .\debuglog.h
.\interlocked.h:	..\include\config.h
.\debuglog.h:	.\errorstr.h
abs2rel.c:	fileutil.h
rel2abs.c:	fileutil.h