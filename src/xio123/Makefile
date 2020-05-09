#
#  xio123.dll makefile
#

TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils
TARGET  = all2

include $(TOPDIR)\config\makerules

OBJECTS = xio$(CO) xio_file$(CO) xio_http$(CO) xio_ftp$(CO) xio_cddb$(CO) xio_protocol$(CO) \
	xio_socket$(CO) xio_syncbuffer$(CO) xio_asyncbuffer$(CO) xio_buffer$(CO)
LIBS    = $(TOPDIR)\utils\cpp\cpputil$(LBO) $(TOPDIR)\utils\utilfct$(LBO)
SYSLIBS = $(LIBS_TCP)
DEF     = xio$(LDEF_SUFFX)

all2: xio123.dll xio123$(LBI) $(MDUMMY)

xio123.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_DLL) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)
	$(POSTLINKCMD)

xio123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ xio123.dll

clean:  $(MDUMMY)
	-@del xio123$(LBI) xio123.dll $(OBJECTS) 2> nul

xio.o:		xio.cpp ..\include\plugin.h  ..\utils\cpp\container\stringmap.h xio_protocol.h xio.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h xio_ftp.h xio_cddb.h xio_asyncbuffer.h
xio_asyncbuffer.o:	xio_asyncbuffer.cpp ..\utils\debuglog.h  xio_asyncbuffer.h
xio_buffer.o:	xio_buffer.cpp ..\utils\cpp\xstring.h xio_buffer.h  xio.h
xio_cddb.o:	xio_cddb.cpp xio_cddb.h xio_protocol.h xio.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h  ..\utils\cpp\url123.h
xio_file.o:	xio_file.cpp ..\utils\eautils.h ..\utils\debuglog.h 
xio_ftp.o:	xio_ftp.cpp xio_socket.h  ..\utils\cpp\url123.h
xio_protocol.o:	xio_protocol.cpp xio_buffer.h 
xio_syncbuffer.o:	xio_syncbuffer.cpp ..\utils\debuglog.h  xio_protocol.h xio.h ..\utils\cpp\xstring.h ..\utils\cpp\mutex.h
xio_http.o:	xio_http.cpp xio_socket.h  ..\utils\cpp\xstring.h ..\utils\utilfct.h ..\utils\errorstr.h ..\utils\strutils.h
xio_socket.o:	xio_socket.cpp xio.h ..\utils\cpp\xstring.h 
