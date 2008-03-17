#
#  libvorbis library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..\include;$(TOPDIR)\ogg123\include;.;modes;$(TOPDIR)\utils

!include $(TOPDIR)\config\makerules

all: libvorbis$(LBO) $(MDUMMY)

OBJECTS = mdct$(CO) smallft$(CO) block$(CO) envelope$(CO) window$(CO) lsp$(CO)
OBJECTS = $(OBJECTS) lpc$(CO) analysis$(CO) synthesis$(CO) psy$(CO) info$(CO)
OBJECTS = $(OBJECTS) floor1$(CO) floor0$(CO) res0$(CO) mapping0$(CO)
OBJECTS = $(OBJECTS) registry$(CO) codebook$(CO) sharedbook$(CO)
OBJECTS = $(OBJECTS) bitrate$(CO) vorbisfile$(CO) vorbisenc$(CO)

libvorbis$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libvorbis$(LBO) 2> nul
	-@del $(OBJECTS)      2> nul
