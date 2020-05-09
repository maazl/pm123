#
#  utilfct library makefile
#

TOPDIR   = ..
INCDIR   = $(TOPDIR)\include;.
TARGET   = utilfct$(LBO)

include $(TOPDIR)\config\makerules

OBJECTS = utilfct$(CO) utilfct_listbox$(CO) utilfct_menu$(CO) utilfct_notebook$(CO) \
	utilfct_dlg$(CO) utilfct_pm_misc$(CO) utilfct_thread$(CO) utilfct_slider$(CO) \
	fileutil$(CO) \
	errorstr$(CO) rel2abs$(CO) abs2rel$(CO) inimacro$(CO) \
	charset$(CO) wildcards$(CO) queue$(CO) strlcpy$(CO) strchomp$(CO) \
	strlcat$(CO) snprintf$(CO) strnchr$(CO) strnlen$(CO) \
	strutils$(CO) strptime$(CO) \
	eautils$(CO) eadecode$(CO) \
	memcmpcpy$(CO) vdelegate$(CO) interlocked$(CO) debuglog$(CO)
STDLIBS = $(LIBS_UNI)

utilfct$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

charset.exe:	charset.obj
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS) $(STDLIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)
	
clean: $(MDUMMY)
	-@del utilfct$(LBO) $(OBJECTS) 2> nul

cpp\algorithm.o:	cpp\algorithm.cpp 
cpp\debugmem.o:	cpp\debugmem.cpp
cpp\directory.o:	cpp\directory.cpp 
cpp\dlgcontrols.o:	cpp\dlgcontrols.cpp 
cpp\event.o:	cpp\event.cpp cpp\cpputil.h
cpp\mutex.o:	cpp\mutex.cpp 
cpp\pmutils_dnd.o:	cpp\pmutils_dnd.cpp 
cpp\priority_queue.o:	cpp\priority_queue.cpp 
cpp\queue.o:	cpp\queue.cpp 
cpp\showaccel.o:	cpp\showaccel.cpp 
cpp\url123.o:	cpp\url123.cpp 
cpp\windowbase.o:	cpp\windowbase.cpp 
cpp\xstring.o:	cpp\xstring.cpp cpp\container\btree.h
cpp\xstring_alloc.o:	cpp\xstring_alloc.cpp 
cpp\xstring_alloc_plugin.o:	cpp\xstring_alloc_plugin.cpp 
cpp\xstringbuilder.o:	cpp\xstringbuilder.cpp 
cpp\xstringconst.o:	cpp\xstringconst.cpp 
cpp\container\btree.o:	cpp\container\btree.cpp cpp\cpputil.h 
cpp\container\iterator.o:	cpp\container\iterator.cpp 
cpp\container\list.o:	cpp\container\list.cpp 
cpp\container\stringmap.o:	cpp\container\stringmap.cpp 
cpp\container\stringset.o:	cpp\container\stringset.cpp 
cpp\container\vector.o:	cpp\container\vector.cpp cpp\cpputil.h
cpp\test\test_btree.o:	cpp\test\test_btree.cpp 
cpp\test\test_int_ptr.o:	cpp\test\test_int_ptr.cpp
utilfct_listbox.o:	utilfct_listbox.c debuglog.h 
wildcards.o:	wildcards.c .\debuglog.h 
abs2rel.o:	abs2rel.c 
charset.o:	charset.c strutils.h utilfct.h .\errorstr.h  .\strutils.h
debuglog.o:	debuglog.c snprintf.h 
eadecode.o:	eadecode.c strutils.h 
eautils.o:	eautils.c 
errorstr.o:	errorstr.c snprintf.h
fileutil.o:	fileutil.c strutils.h 
inimacro.o:	inimacro.c fileutil.h  debuglog.h
interlocked.o:	interlocked.c 
memcmpcpy.o:	memcmpcpy.c 
queue.o:	queue.c 
rel2abs.o:	rel2abs.c 
snprintf.o:	snprintf.c
strchomp.o:	strchomp.c 
strlcat.o:	strlcat.c
strlcpy.o:	strlcpy.c
strnchr.o:	strnchr.c 
strnlen.o:	strnlen.c 
strptime.o:	strptime.c 
strutils.o:	strutils.c 
utilfct.o:	utilfct.c debuglog.h 
utilfct_dlg.o:	utilfct_dlg.c debuglog.h 
utilfct_slider.o:	utilfct_slider.c debuglog.h 
utilfct_menu.o:	utilfct_menu.c debuglog.h 
utilfct_thread.o:	utilfct_thread.c debuglog.h 
utilfct_notebook.o:	utilfct_notebook.c debuglog.h 
utilfct_pm_misc.o:	utilfct_pm_misc.c debuglog.h 
vdelegate.o:	vdelegate.c 
