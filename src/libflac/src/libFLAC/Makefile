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

include $(TOPDIR)\config\makerules

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
OBJECTS+=ia32\bitreader_asm$(OBJ) \
	ia32\cpu_asm$(OBJ) \
	ia32\fixed_asm$(OBJ) \
	ia32\lpc_asm$(OBJ) \
	ia32\stream_encoder_asm$(OBJ)

libFLAC$(LBO): $(OBJECTS) $(LIBS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libFLAC$(LBO) $(OBJECTS) *.e 2>nul

bitmath.o:	bitmath.c ..\..\../libflac/include\FLAC/assert.h 
bitreader.o:	bitreader.c .\include\private\bitreader.h  ..\..\../libflac/include\FLAC/assert.h
bitwriter.o:	bitwriter.c .\include\private\bitwriter.h  ..\..\../libflac/include\FLAC/assert.h
cpu.o:		cpu.c 
crc.o:		crc.c 
fixed.o:	fixed.c .\include\private\fixed.h 
float.o:	float.c .\include\private\float.h 
format.o:	format.c ..\..\../libflac/include\FLAC/format.h ..\..\include\FLAC\ordinals.h 
lpc.o:		lpc.c ..\..\../libflac/include\FLAC/format.h ..\..\include\FLAC\ordinals.h .\include\private\lpc.h 
md5.o:		md5.c ..\..\../libflac/include\share/alloc.h
memory.o:	memory.c ..\..\../libflac/include\FLAC/assert.h 
metadata_iterators.o:	metadata_iterators.c ..\..\../libflac/include\FLAC/assert.h  ..\..\../libflac/include\share/alloc.h
metadata_object.o:	metadata_object.c ..\..\../libflac/include\FLAC/assert.h 
ogg_decoder_aspect.o:	ogg_decoder_aspect.c .\include\private\ogg_decoder_aspect.h 
ogg_encoder_aspect.o:	ogg_encoder_aspect.c .\include\private\ogg_encoder_aspect.h 
ogg_helper.o:	ogg_helper.c ..\..\../libflac/include\share/alloc.h .\include\protected\stream_encoder.h 
ogg_mapping.o:	ogg_mapping.c 
stream_decoder.o:	stream_decoder.c ..\..\../libflac/include\share/alloc.h .\include\private\bitreader.h  .\include\private\cpu.h .\include\private\fixed.h .\include\private\lpc.h .\include\private\memory.h
stream_encoder.o:	stream_encoder.c ..\..\../libflac/include\FLAC/stream_decoder.h ..\..\include\FLAC\format.h ..\..\include\FLAC\ordinals.h .\include\protected\stream_encoder.h  .\include\private\bitmath.h .\include\private\cpu.h .\include\private\format.h .\include\private\md5.h .\include\private\ogg_helper.h .\include\private\stream_encoder_framing.h
stream_encoder_framing.o:	stream_encoder_framing.c .\include\private\crc.h 
window.o:	window.c ..\..\../libflac/include\FLAC/format.h ..\..\include\FLAC\ordinals.h 
