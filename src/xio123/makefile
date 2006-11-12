#
#  xio123.dll makefile
#

DLL     = yes
TOPDIR  = ..
INCDIR  = $(TOPDIR)\include;$(TOPDIR)\utils

!include $(TOPDIR)\config\makerules

OBJECTS = xio$(CO) xio_file$(CO) xio_http$(CO) xio_ftp$(CO) xio_buffer$(CO) xio_url$(CO)
OBJECTS = $(OBJECTS) xio_socket$(CO)
LIBS    = $(LIBS_TCP) $(TOPDIR)\utils\utilfct$(LBO)
DEF     = xio$(LDEF_SUFFX)

all: xio123.dll xio123$(LBI) $(MDUMMY)

xio123.dll: $(OBJECTS) $(DEF)
	$(CL) $(LFLAGS) $(LFLAGS_OUT)$@ $(LOBJ_PREFX)$(OBJECTS) $(LIBS)$(LOBJ_SUFFX) $(LDEF_PREFX)$(DEF)

xio123$(LBI): $(DEF)
	$(LI) $@ xio123.dll

xio$(CO):        xio.c        xio.h xio_protocol.h xio_socket.h xio_file.h xio_http.h xio_ftp.h xio_buffer.h
xio_buffer$(CO): xio_buffer.c xio.h xio_protocol.h xio_buffer.h
xio_file$(CO):   xio_file.c   xio.h xio_protocol.h xio_file.h
xio_ftp$(CO):    xio_ftp.c    xio.h xio_protocol.h xio_url.h xio_socket.h xio_ftp.h
xio_http$(CO):   xio_http.c   xio.h xio_protocol.h xio_url.h xio_socket.h xio_http.h
xio_url$(CO):    xio_url.c    xio_url.h
xio_socket$(CO): xio_socket.c xio.h xio_socket.h

clean: $(MDUMMY)
	-@del xio123$(LBI) xio123.dll $(OBJECTS) 2> nul
