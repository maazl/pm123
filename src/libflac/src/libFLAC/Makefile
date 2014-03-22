#
#  libFLAC.lib makefile
#

VERSION=\"1.2.1\"
TOPDIR   = ..\..\..
INCDIR   = ./include;$(TOPDIR)/libflac/include;$(TOPDIR)/ogg123/include;$(TOPDIR)/include;$(TOPDIR)/utils
TARGET   = libFLAC$(LBO)
CFLAGS_USR = -DFLAC__CPU_IA32 -DFLAC__USE_3DNOW -DFLAC__HAS_NASM -DFLAC__ALIGN_MALLOC_DATA \
	-DVERSION=$(VERSION) -DHAVE_CONFIG_H \
	-Wmissing-prototypes -Wstrict-prototypes -Winline
CFLAGS_DBG_USR = -DFLAC__OVERFLOW_DETECT
CFLAGS_OPT_USR = -DFLaC__INLINE=INLINE
AFLAGS_USR = -DOBJ_FORMAT_aout

!include $(TOPDIR)\config\makerules

# C files
OBJECTS=bitmath$(CO) \
	bitreader$(CO) \
	bitwriter$(CO) \
	cpu$(CO) \
	crc$(CO) \
	fixed$(CO) \
	float$(CO) \
	format$(CO) \
	lpc$(CO) \
	md5$(CO) \
	memory$(CO) \
	metadata_iterators$(CO) \
	metadata_object$(CO) \
	ogg_decoder_aspect$(CO) \
	ogg_encoder_aspect$(CO) \
	ogg_helper$(CO) \
	ogg_mapping$(CO) \
	stream_decoder$(CO) \
	stream_encoder$(CO) \
	stream_encoder_framing$(CO) \
	window$(CO)

# Assembler files
OBJECTS=$(OBJECTS) \
	ia32/bitreader_asm$(OBJ) \
	ia32/cpu_asm$(OBJ) \
	ia32/fixed_asm$(OBJ) \
	ia32/lpc_asm$(OBJ) \
	ia32/stream_encoder_asm$(OBJ)

libFLAC$(LBO): $(OBJECTS) $(LIBS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libFLAC$(LBO) $(OBJECTS) *.e 2> nul

bitmath.c:	.\include\private\bitmath.h ..\..\include\FLAC\assert.h
.\include\private\bitmath.h:	..\..\include\FLAC\ordinals.h
bitreader.c:	.\include\private\bitmath.h .\include\private\bitreader.h .\include\private\crc.h ..\..\include\FLAC\assert.h
.\include\private\bitreader.h:	..\..\include\FLAC\ordinals.h
.\include\private\crc.h:	..\..\include\FLAC\ordinals.h
bitwriter.c:	.\include\private\bitmath.h .\include\private\bitwriter.h .\include\private\crc.h ..\..\include\FLAC\assert.h ..\..\include\share\alloc.h
.\include\private\bitwriter.h:	..\..\include\FLAC\ordinals.h
cpu.c:		.\include\private\cpu.h
.\include\private\cpu.h:	..\..\include\FLAC\ordinals.h
crc.c:		.\include\private\crc.h
fixed.c:	.\include\private\bitmath.h .\include\private\fixed.h ..\..\include\FLAC\assert.h
.\include\private\fixed.h:	.\include\private\float.h ..\..\include\FLAC\format.h
.\include\private\float.h:	..\..\include\FLAC\ordinals.h
float.c:	..\..\include\FLAC\assert.h .\include\private\float.h
format.c:	..\..\include\FLAC\assert.h ..\..\include\FLAC\format.h .\include\private\format.h
.\include\private\format.h:	..\..\include\FLAC\format.h
lpc.c:		..\..\include\FLAC\assert.h ..\..\include\FLAC\format.h .\include\private\bitmath.h .\include\private\lpc.h
.\include\private\lpc.h:	.\include\private\float.h ..\..\include\FLAC\format.h
md5.c:		.\include\private\md5.h ..\..\include\share\alloc.h
.\include\private\md5.h:	..\..\include\FLAC\ordinals.h
memory.c:	.\include\private\memory.h ..\..\include\FLAC\assert.h ..\..\include\share\alloc.h
.\include\private\memory.h:	.\include\private\float.h ..\..\include\FLAC\ordinals.h
metadata_iterators.c:	.\include\private\metadata.h ..\..\include\FLAC\assert.h ..\..\include\FLAC\stream_decoder.h ..\..\include\share\alloc.h
.\include\private\metadata.h:	..\..\include\FLAC\metadata.h
metadata_object.c:	.\include\private\metadata.h ..\..\include\FLAC\assert.h ..\..\include\share\alloc.h
ogg_decoder_aspect.c:	..\..\include\FLAC\assert.h .\include\private\ogg_decoder_aspect.h .\include\private\ogg_mapping.h
.\include\private\ogg_decoder_aspect.h:	..\..\include\FLAC\ordinals.h ..\..\include\FLAC\stream_decoder.h
.\include\private\ogg_mapping.h:	..\..\include\FLAC\ordinals.h
ogg_encoder_aspect.c:	..\..\include\FLAC\assert.h .\include\private\ogg_encoder_aspect.h .\include\private\ogg_mapping.h
.\include\private\ogg_encoder_aspect.h:	..\..\include\FLAC\ordinals.h ..\..\include\FLAC\stream_encoder.h
ogg_helper.c:	..\..\include\FLAC\assert.h ..\..\include\share\alloc.h .\include\private\ogg_helper.h .\include\protected\stream_encoder.h
.\include\private\ogg_helper.h:	..\..\include\FLAC\stream_encoder.h
.\include\protected\stream_encoder.h:	..\..\include\FLAC\stream_encoder.h .\include\private\ogg_encoder_aspect.h .\include\private\float.h
ogg_mapping.c:	.\include\private\ogg_mapping.h
stream_decoder.c:	..\..\include\FLAC\assert.h ..\..\include\share\alloc.h .\include\protected\stream_decoder.h .\include\private\bitreader.h .\include\private\bitmath.h .\include\private\cpu.h .\include\private\crc.h .\include\private\fixed.h .\include\private\format.h .\include\private\lpc.h .\include\private\md5.h .\include\private\memory.h
.\include\protected\stream_decoder.h:	..\..\include\FLAC\stream_decoder.h .\include\private\ogg_decoder_aspect.h
stream_encoder.c:	..\..\include\FLAC\assert.h ..\..\include\FLAC\stream_decoder.h ..\..\include\share\alloc.h .\include\protected\stream_encoder.h .\include\private\bitwriter.h .\include\private\bitmath.h .\include\private\crc.h .\include\private\cpu.h .\include\private\fixed.h .\include\private\format.h .\include\private\lpc.h .\include\private\md5.h .\include\private\memory.h .\include\private\ogg_helper.h .\include\private\ogg_mapping.h .\include\private\stream_encoder_framing.h .\include\private\window.h
.\include\private\stream_encoder_framing.h:	..\..\include\FLAC\format.h
.\include\private\window.h:	.\include\private\float.h ..\..\include\FLAC\format.h
stream_encoder_framing.c:	.\include\private\stream_encoder_framing.h .\include\private\crc.h ..\..\include\FLAC\assert.h
window.c:	..\..\include\FLAC\assert.h ..\..\include\FLAC\format.h .\include\private\window.h
