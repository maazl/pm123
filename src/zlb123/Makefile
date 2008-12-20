#
#  zlb123.dll makefile
#

TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils
LIBS    = $(TOPDIR)\utils\utilfct$(LBO)

!include $(TOPDIR)\config\makerules

OBJECTS = adler32$(CO) compress$(CO) crc32$(CO) gzio$(CO) uncompr$(CO) deflate$(CO)
OBJECTS = $(OBJECTS) trees$(CO) zutil$(CO) inflate$(CO) infback$(CO) inftrees$(CO) inffast$(CO)
DEF     = zlib$(LDEF_SUFFX)

all: zlb123.dll zlb123$(LBI) $(MDUMMY)

zlb123.dll: $(OBJECTS) $(LIBS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

zlb123$(LBI): zlb123.dll $(DEF)
	$(LI) $(IFLAGS_OUT)$@ zlb123.dll

clean: $(MDUMMY)
	-@del zlb123$(LBI) zlb123.dll $(OBJECTS) 2> nul
