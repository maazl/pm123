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
OBJECTS = $(OBJECTS) registry$(CO) codebook$(CO) sharedbook$(CO) lookup$(CO)
OBJECTS = $(OBJECTS) bitrate$(CO) vorbisfile$(CO)
# vorbisenc$(CO)

libvorbis$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libvorbis$(LBO) 2> nul
	-@del $(OBJECTS)      2> nul

analysis.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h registry.h scales.h os.h misc.h
barkmel.c:	scales.h
bitrate.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h os.h misc.h bitrate.h
block.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h window.h mdct.h lpc.h registry.h misc.h
codebook.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codebook.h scales.h misc.h os.h
envelope.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h os.h scales.h envelope.h mdct.h misc.h
floor0.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h registry.h lpc.h lsp.h codebook.h scales.h misc.h os.h
floor1.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h registry.h codebook.h misc.h scales.h
info.c:		..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h codebook.h registry.h window.h psy.h misc.h os.h
lookup.c:	lookup.h lookup_data.h os.h misc.h
lpc.c:		os.h smallft.h lpc.h scales.h misc.h
lsp.c:		lsp.h os.h misc.h lookup.h scales.h lookup.c
mapping0.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h codebook.h window.h registry.h psy.h misc.h os.h lpc.h lsp.h envelope.h mdct.h scales.h
mdct.c:		..\include\vorbis/codec.h mdct.h os.h misc.h
misc.c:		..\include\vorbis/codec.h misc.h
psy.c:		..\include\vorbis/codec.h codec_internal.h masking.h psy.h os.h lpc.h smallft.h scales.h misc.h
psytune.c:	..\include\vorbis/codec.h codec_internal.h os.h misc.h psy.h mdct.h smallft.h window.h scales.h lpc.h lsp.h masking.h registry.h
registry.c:	..\include\vorbis/codec.h codec_internal.h registry.h misc.h
res0.c:		..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h registry.h codebook.h misc.h os.h
sharedbook.c:	..\..\ogg123\include\ogg/ogg.h os.h misc.h ..\include\vorbis/codec.h codebook.h scales.h
smallft.c:	smallft.h os.h misc.h
synthesis.c:	..\..\ogg123\include\ogg/ogg.h ..\include\vorbis/codec.h codec_internal.h registry.h misc.h os.h
vorbisenc.c:	..\include\vorbis/codec.h ..\include\vorbis/vorbisenc.h codec_internal.h os.h misc.h modes/setup_44.h modes/setup_44u.h modes/setup_32.h modes/setup_8.h modes/setup_11.h modes/setup_16.h modes/setup_22.h modes/setup_X.h
vorbisfile.c:	..\include\vorbis/codec.h ..\include\vorbis/vorbisfile.h os.h misc.h
window.c:	os.h misc.h

codec_internal.h:	envelope.h codebook.h psy.h bitrate.h highlevel.h
envelope.h:	mdct.h
codebook.h:	..\..\ogg123\include\ogg/ogg.h
psy.h:		smallft.h backends.h envelope.h
bitrate.h:	..\include\vorbis/codec.h os.h
os.h:		..\..\include\config.h ..\..\ogg123\include\ogg/os_types.h misc.h
scales.h:	os.h
modes/setup_44.h:	modes/floor_all.h modes/residue_44.h modes/psych_44.h
modes/floor_all.h:	..\include\vorbis/codec.h backends.h books/floor/floor_books.h
books/floor/floor_books.h:	codebook.h
modes/residue_44.h:	..\include\vorbis/codec.h backends.h books/coupled/res_books_stereo.h
books/coupled/res_books_stereo.h:	codebook.h
modes/setup_44u.h:	modes/residue_44u.h
modes/residue_44u.h:	..\include\vorbis/codec.h backends.h books/uncoupled/res_books_uncoupled.h
books/uncoupled/res_books_uncoupled.h:	codebook.h
mdct.h:		..\include\vorbis/codec.h
smallft.h:	..\include\vorbis/codec.h
misc.h:		..\include\vorbis/codec.h
lpc.h:		..\include\vorbis/codec.h
modes/setup_8.h:	modes/psych_8.h modes/residue_8.h
modes/residue_8.h:	..\include\vorbis/codec.h backends.h
modes/setup_11.h:	modes/psych_11.h
modes/setup_16.h:	modes/psych_16.h modes/residue_16.h
