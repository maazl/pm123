#
#  xio123.dll makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils

!include $(TOPDIR)\config\makerules

OBJECTS = xio$(CO) xio_file$(CO) xio_http$(CO) xio_ftp$(CO) xio_cddb$(CO) xio_buffer$(CO) xio_url$(CO)
OBJECTS = $(OBJECTS) xio_socket$(CO)
LIBS    = $(TOPDIR)\utils\utilfct$(LBO)
SYSLIBS = $(LIBS_TCP)
DEF     = xio$(LDEF_SUFFX)

all: xio123.dll xio123$(LBI) $(MDUMMY)

xio123.dll: $(OBJECTS) $(DEF) $(LIBS)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS)$(LOBJ_SUFFX) $(LLIB_PREFX)$(LIBS) $(SYSLIBS)$(LLIB_SUFFX) $(LDEF_PREFX)$(DEF)

xio123$(LBI): $(DEF)
	$(LI) $(IFLAGS_OUT)$@ xio123.dll

clean:  $(MDUMMY)
	-@del xio123$(LBI) xio123.dll $(OBJECTS) 2> nul
depend: $(MDUMMY)
	@$(TOPDIR)\config\makedep.cmd *.c -r -x -I$(INCDIR)

xio.c:		..\utils\utilfct.h ..\include\decoder_plug.h ..\utils\debuglog.h xio.h xio_protocol.h xio_buffer.h xio_file.h xio_ftp.h xio_http.h xio_cddb.h xio_socket.h
xio_protocol.h:	xio.h
xio_buffer.h:	xio.h
xio_file.h:	xio.h xio_protocol.h
xio_ftp.h:	xio.h xio_protocol.h
xio_http.h:	xio.h xio_protocol.h
xio_cddb.h:	xio.h xio_protocol.h
xio_buffer.c:	..\utils\utilfct.h ..\utils\debuglog.h xio_buffer.h xio_protocol.h
xio_cddb.c:	..\utils\snprintf.h xio_cddb.h xio_url.h xio_socket.h
xio_file.c:	xio_file.h xio_url.h ..\utils\debuglog.h
xio_ftp.c:	xio_ftp.h xio_url.h xio_socket.h
xio_http.c:	xio_http.h xio_url.h xio_socket.h xio.h ..\utils\utilfct.h
xio_socket.c:	xio_socket.h xio.h
xio_url.c:	xio_url.h ..\utils\utilfct.h
xio.h:		..\include\config.h
