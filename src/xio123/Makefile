#
#  xio123.dll makefile
#

TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils
TARGET  = all2

!include $(TOPDIR)\config\makerules

OBJECTS = xio$(CO) xio_file$(CO) xio_http$(CO) xio_ftp$(CO) xio_cddb$(CO) xio_protocol$(CO)
OBJECTS = $(OBJECTS) xio_url$(CO) xio_socket$(CO) xio_syncbuffer$(CO) xio_asyncbuffer$(CO) xio_buffer$(CO)
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

xio.cpp:	..\utils\utilfct.h ..\include\decoder_plug.h ..\utils\debuglog.h xio.h xio_protocol.h xio_file.h xio_ftp.h xio_http.h xio_cddb.h xio_syncbuffer.h xio_asyncbuffer.h xio_socket.h
xio_protocol.h:	..\include\config.h ..\utils\cpp/cpputil.h ..\utils\cpp/mutex.h
xio_buffer.h:	xio_protocol.h
xio_file.h:	xio.h xio_protocol.h
xio_ftp.h:	xio.h xio_protocol.h
xio_http.h:	xio.h xio_protocol.h
xio_cddb.h:	xio.h xio_protocol.h
xio_buffer.cpp:	..\utils\utilfct.h ..\utils\debuglog.h xio_buffer.h xio_protocol.h xio.h
xio_cddb.cpp:	..\utils\snprintf.h xio_cddb.h xio_url.h xio_socket.h ..\utils\debuglog.h
xio_file.cpp:	xio_file.h xio_url.h ..\utils\debuglog.h
xio_ftp.cpp:	xio_ftp.h xio_url.h xio_socket.h ..\utils\interlocked.h
xio_http.cpp:	xio_http.h xio_url.h xio_socket.h xio.h ..\utils\utilfct.h
xio_socket.cpp:	xio_socket.h xio.h ..\utils\debuglog.h
xio_url.cpp:	xio_url.h ..\utils\utilfct.h ..\utils\debuglog.h
xio.h:		..\include\config.h
xio_syncbuffer.h:	xio_buffer.h
xio_asyncbuffer.h:	xio_buffer.h ..\utils\cpp/mutex.h
xio_asyncbuffer.cpp:	..\utils\utilfct.h ..\utils\debuglog.h xio_protocol.h xio_asyncbuffer.h xio.h
xio_protocol.cpp:	xio_protocol.h xio_buffer.h ..\utils\utilfct.h
xio_syncbuffer.cpp:	..\utils\utilfct.h ..\utils\debuglog.h xio_syncbuffer.h xio_protocol.h xio.h
