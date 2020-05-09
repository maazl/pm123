#
#  libvorbis library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..\include;$(TOPDIR)\ogg123\include;.;modes;$(TOPDIR)\utils

include $(TOPDIR)\config\makerules

all: libvorbis$(LBO) $(MDUMMY)

OBJECTS = mdct$(CO) smallft$(CO) block$(CO) envelope$(CO) window$(CO) lsp$(CO) \
	lpc$(CO) analysis$(CO) synthesis$(CO) psy$(CO) info$(CO) \
	floor1$(CO) floor0$(CO) res0$(CO) mapping0$(CO) \
	registry$(CO) codebook$(CO) sharedbook$(CO) lookup$(CO) \
	bitrate$(CO) vorbisfile$(CO)
# vorbisenc$(CO)

libvorbis$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libvorbis$(LBO) 2> nul
	-@del $(OBJECTS)      2> nul

vorbisfile.o:	vorbisfile.c ..\include\vorbis\vorbisfile.h  misc.h
analysis.o:	analysis.c ..\include\vorbis\codec.h registry.h os.h ..\..\ogg123\include\ogg/os_types.h 
barkmel.o:	barkmel.c 
bitrate.o:	bitrate.c ..\include\vorbis\codec.h os.h ..\..\ogg123\include\ogg/os_types.h  bitrate.h codec_internal.h codebook.h
block.o:	block.c ..\include\vorbis\codec.h window.h lpc.h  misc.h
codebook.o:	codebook.c ..\include\vorbis\codec.h scales.h  os.h ..\..\ogg123\include\ogg/os_types.h
envelope.o:	envelope.c ..\include\vorbis\codec.h os.h ..\..\ogg123\include\ogg/os_types.h  envelope.h misc.h
floor0.o:	floor0.c ..\include\vorbis\codec.h registry.h lsp.h scales.h  os.h ..\..\ogg123\include\ogg/os_types.h
floor1.o:	floor1.c ..\include\vorbis\codec.h registry.h misc.h 
info.o:		info.c ..\include\vorbis\codec.h codebook.h  window.h misc.h
lookup.o:	lookup.c lookup_data.h misc.h 
lpc.o:		lpc.c smallft.h  scales.h
lsp.o:		lsp.c os.h ..\..\ogg123\include\ogg/os_types.h  lookup.h lookup.c lookup_data.h misc.h
mapping0.o:	mapping0.c ..\include\vorbis\codec.h codebook.h  registry.h misc.h lpc.h envelope.h scales.h
mdct.o:		mdct.c mdct.h  misc.h
psy.o:		psy.c codec_internal.h codebook.h  bitrate.h psy.h backends.h lpc.h scales.h
psytune.o:	psytune.c codec_internal.h codebook.h  bitrate.h misc.h mdct.h window.h lpc.h masking.h
registry.o:	registry.c codec_internal.h codebook.h  bitrate.h misc.h
res0.o:		res0.c ..\include\vorbis\codec.h registry.h misc.h 
sharedbook.o:	sharedbook.c os.h ..\..\ogg123\include\ogg/os_types.h  ..\include\vorbis\codec.h scales.h
smallft.o:	smallft.c os.h ..\..\ogg123\include\ogg/os_types.h 
synthesis.o:	synthesis.c ..\include\vorbis\codec.h registry.h os.h ..\..\ogg123\include\ogg/os_types.h 
tone.o:		tone.c
vorbisenc.o:	vorbisenc.c ..\include\vorbis\vorbisenc.h  os.h ..\..\ogg123\include\ogg/os_types.h modes\setup_44.h modes\setup_44p51.h modes\setup_8.h modes\residue_8.h modes\setup_16.h modes\residue_16.h modes\setup_X.h
window.o:	window.c misc.h 
