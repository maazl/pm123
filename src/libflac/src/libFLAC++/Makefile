#
#  libFLAC.lib makefile
#

VERSION=\"1.2.1\"
TOPDIR   = ..\..\..
INCDIR   = $(TOPDIR)/libflac/include;$(TOPDIR)/include;
TARGET   = libFLAC++$(LBO)
CFLAGS_USR = -DVERSION=$(VERSION) -DHAVE_CONFIG_H \
	-Wmissing-prototypes -Wstrict-prototypes -Winline
CFLAGS_DBG_USR = -DFLAC__OVERFLOW_DETECT
CFLAGS_OPT_USR = -DFLaC__INLINE=INLINE

!include $(TOPDIR)\config\makerules

# C files
OBJECTS=	metadata$(CO) \
	stream_decoder$(CO) \
	stream_encoder$(CO)

libFLAC++$(LBO): $(OBJECTS) $(LIBS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libFLAC$(LBO) $(OBJECTS) *.e 2> nul

