#
#  sndfile library makefile
#

TOPDIR  = ..\..
INCDIR  = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET  = all2

include $(TOPDIR)\config\makerules

all2: sndfile$(LBO) $(MDUMMY)

OBJECTS = sndfile$(CO) common$(CO) file_io$(CO) command$(CO) pcm$(CO) ulaw$(CO) \
	alaw$(CO) float32$(CO) double64$(CO) ima_adpcm$(CO) \
	ms_adpcm$(CO) gsm610$(CO) dwvw$(CO) vox_adpcm$(CO) \
	interleave$(CO) strings$(CO) dither$(CO) broadcast$(CO) \
	aiff$(CO) au$(CO) g72x$(CO) avr$(CO) caf$(CO) ircam$(CO) \
	macbinary3$(CO) macos$(CO) mat4$(CO) mat5$(CO) nist$(CO) \
	paf$(CO) pvf$(CO) raw$(CO) svx$(CO) voc$(CO) htk$(CO) \
	dwd$(CO) ogg$(CO) rx2$(CO) sd2$(CO) sds$(CO) txw$(CO) \
	wve$(CO) w64$(CO) wav_w64$(CO) wav$(CO) xi$(CO) flac$(CO) \
	chanmap$(CO) id3$(CO) rf64$(CO) chunk$(CO) audio_detect$(CO) \
	mpc2k$(CO) ima_oki_adpcm$(CO)

sndfile$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del sndfile$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul

ms_adpcm.o:	ms_adpcm.c sndfile.h  common.h
nist.o:		nist.c sndfile.h  common.h
paf.o:		paf.c sndfile.h  common.h
wve.o:		wve.c sndfile.h  common.h
interleave.o:	interleave.c sndfile.h 
xi.o:		xi.c sndfile.h  common.h
aiff.o:		aiff.c sndfile.h  common.h
pcm.o:		pcm.c sndfile.h  common.h
alaw.o:		alaw.c sndfile.h 
pvf.o:		pvf.c sndfile.h  common.h
au.o:		au.c sndfile.h  common.h
ulaw.o:		ulaw.c sndfile.h 
ircam.o:	ircam.c sndfile.h  common.h
audio_detect.o:	audio_detect.c common.h sndfile.h 
avr.o:		avr.c sndfile.h  common.h
mpc2k.o:	mpc2k.c sndfile.h  common.h
voc.o:		voc.c sndfile.h  common.h
broadcast.o:	broadcast.c common.h sndfile.h 
raw.o:		raw.c sndfile.h 
caf.o:		caf.c sndfile.h  common.h
vox_adpcm.o:	vox_adpcm.c sndfile.h  common.h
chanmap.o:	chanmap.c sndfile.h  chanmap.h
rf64.o:		rf64.c sndfile.h  common.h
rx2.o:		rx2.c sndfile.h  common.h
chunk.o:	chunk.c sndfile.h  common.h
sd2.o:		sd2.c sndfile.h  common.h
command.o:	command.c sndfile.h 
sds.o:		sds.c sndfile.h  common.h
common.o:	common.c sndfile.h  common.h
dither.o:	dither.c sndfile.h  common.h
sndfile.o:	sndfile.c sndfile.h  common.h
double64.o:	double64.c sndfile.h  common.h
dwd.o:		dwd.c sndfile.h  common.h
w64.o:		w64.c sndfile.h  common.h
dwvw.o:		dwvw.c sndfile.h  common.h
file_io.o:	file_io.c ..\..\include\sys/io.h common.h sndfile.h 
strings.o:	strings.c sndfile.h 
flac.o:		flac.c sndfile.h 
svx.o:		svx.c sndfile.h  common.h
float32.o:	float32.c sndfile.h  common.h
test_broadcast_var.o:	test_broadcast_var.c common.h sndfile.h 
test_audio_detect.o:	test_audio_detect.c common.h sndfile.h  test_main.h
g72x.o:		g72x.c sndfile.h  common.h
test_conversions.o:	test_conversions.c common.h sndfile.h 
test_endswap.o:	test_endswap.c common.h sndfile.h  test_main.h
gsm610.o:	gsm610.c sndfile.h  common.h GSM610\gsm.h
wav.o:		wav.c sndfile.h  common.h
htk.o:		htk.c sndfile.h  common.h
id3.o:		id3.c sndfile.h  common.h
wav_w64.o:	wav_w64.c sndfile.h  common.h
ima_adpcm.o:	ima_adpcm.c sndfile.h  common.h
test_file_io.o:	test_file_io.c common.h sndfile.h 
ima_oki_adpcm.o:	ima_oki_adpcm.c common.h sndfile.h 
test_log_printf.o:	test_log_printf.c common.h sndfile.h 
macbinary3.o:	macbinary3.c sndfile.h  common.h
test_float.o:	test_float.c common.h sndfile.h 
test_ima_oki_adpcm.o:	test_ima_oki_adpcm.c test_main.h 
macos.o:	macos.c sndfile.h  common.h
test_main.o:	test_main.c test_main.h
mat4.o:		mat4.c sndfile.h  common.h
test_strncpy_crlf.o:	test_strncpy_crlf.c common.h sndfile.h 
txw.o:		txw.c sndfile.h  common.h
mat5.o:		mat5.c sndfile.h  common.h
ogg.o:		ogg.c sndfile.h  common.h
windows.o:	windows.c sndfile.h 
G72x\g721.o:	G72x\g721.c G72x\g72x_priv.h
G72x\g723_16.o:	G72x\g723_16.c G72x\g72x_priv.h
G72x\g723_24.o:	G72x\g723_24.c G72x\g72x_priv.h
G72x\g723_40.o:	G72x\g723_40.c G72x\g72x_priv.h
G72x\g72x.o:	G72x\g72x.c G72x\g72x_priv.h
G72x\g72x_test.o:	G72x\g72x_test.c G72x\g72x_priv.h
GSM610\add.o:	GSM610\add.c 
GSM610\code.o:	GSM610\code.c 
GSM610\decode.o:	GSM610\decode.c 
GSM610\gsm_create.o:	GSM610\gsm_create.c GSM610\gsm.h 
GSM610\gsm_decode.o:	GSM610\gsm_decode.c GSM610\gsm.h
GSM610\gsm_destroy.o:	GSM610\gsm_destroy.c GSM610\config.h
GSM610\gsm_encode.o:	GSM610\gsm_encode.c GSM610\gsm.h
GSM610\gsm_option.o:	GSM610\gsm_option.c GSM610\gsm.h
GSM610\long_term.o:	GSM610\long_term.c 
GSM610\lpc.o:	GSM610\lpc.c 
GSM610\preprocess.o:	GSM610\preprocess.c 
GSM610\rpe.o:	GSM610\rpe.c 
GSM610\short_term.o:	GSM610\short_term.c 
GSM610\table.o:	GSM610\table.c 
