#
#  sndfile library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = all2

!include $(TOPDIR)\config\makerules

all2: sndfile$(LBO) $(MDUMMY)

OBJECTS = sndfile$(CO) common$(CO) file_io$(CO) command$(CO) pcm$(CO) ulaw$(CO)
OBJECTS = $(OBJECTS) alaw$(CO) float32$(CO) double64$(CO) ima_adpcm$(CO)
OBJECTS = $(OBJECTS) ms_adpcm$(CO) gsm610$(CO) dwvw$(CO) vox_adpcm$(CO)
OBJECTS = $(OBJECTS) interleave$(CO) strings$(CO) dither$(CO) broadcast$(CO)
OBJECTS = $(OBJECTS) aiff$(CO) au$(CO) g72x$(CO) avr$(CO) caf$(CO) ircam$(CO)
OBJECTS = $(OBJECTS) macbinary3$(CO) macos$(CO) mat4$(CO) mat5$(CO) nist$(CO)
OBJECTS = $(OBJECTS) paf$(CO) pvf$(CO) raw$(CO) svx$(CO) voc$(CO) htk$(CO)
OBJECTS = $(OBJECTS) dwd$(CO) ogg$(CO) rx2$(CO) sd2$(CO) sds$(CO) txw$(CO)
OBJECTS = $(OBJECTS) wve$(CO) w64$(CO) wav_w64$(CO) wav$(CO) xi$(CO) flac$(CO)
OBJECTS = $(OBJECTS) chanmap$(CO) id3$(CO) rf64$(CO) chunk$(CO) audio_detect$(CO)
OBJECTS = $(OBJECTS) mpc2k$(CO) ima_oki_adpcm$(CO)

sndfile$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del sndfile$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul

aiff.c:		sfconfig.h sndfile.h sfendian.h common.h chanmap.h
sfendian.h:	sfconfig.h
common.h:	sfconfig.h sndfile.h
alaw.c:		sfconfig.h sndfile.h common.h
au.c:		sfconfig.h sndfile.h sfendian.h common.h
audio_detect.c:	sfconfig.h common.h
avr.c:		sfconfig.h sndfile.h sfendian.h common.h
broadcast.c:	sfconfig.h common.h
caf.c:		sfconfig.h sndfile.h sfendian.h common.h chanmap.h
chanmap.c:	sfconfig.h sndfile.h common.h chanmap.h
chunk.c:	sfconfig.h sndfile.h sfendian.h common.h
command.c:	sfconfig.h sndfile.h common.h
common.c:	..\..\include\config.h sndfile.h sfendian.h common.h
..\..\include\config.h:	..\..\include\config_gcc.h ..\..\include\config_wcc.h ..\..\include\config_icc_36.h ..\..\include\config_icc_30.h
dither.c:	sfconfig.h sndfile.h sfendian.h common.h
double64.c:	sfconfig.h sndfile.h sfendian.h common.h
dwd.c:		sfconfig.h sndfile.h sfendian.h common.h
dwvw.c:		sfconfig.h sndfile.h sfendian.h common.h
file_io.c:	sfconfig.h ..\..\include\sys\io.h sndfile.h common.h
flac.c:		sfconfig.h sndfile.h common.h
float32.c:	sfconfig.h sndfile.h sfendian.h common.h
g72x.c:		sfconfig.h sndfile.h sfendian.h common.h G72x/g72x.h
gsm610.c:	sfconfig.h sndfile.h sfendian.h common.h wav_w64.h GSM610/gsm.h
htk.c:		sfconfig.h sndfile.h sfendian.h common.h
id3.c:		sfconfig.h sndfile.h sfendian.h common.h
ima_adpcm.c:	sfconfig.h sndfile.h sfendian.h common.h
ima_oki_adpcm.c:	sfconfig.h common.h ima_oki_adpcm.h
interleave.c:	sfendian.h sndfile.h common.h
ircam.c:	sfconfig.h sndfile.h sfendian.h common.h
macbinary3.c:	sfconfig.h sndfile.h sfendian.h common.h
macos.c:	sfconfig.h sndfile.h sfendian.h common.h
mat4.c:		sfconfig.h sndfile.h sfendian.h common.h
mat5.c:		sfconfig.h sndfile.h sfendian.h common.h
mpc2k.c:	sfconfig.h sndfile.h sfendian.h common.h
ms_adpcm.c:	sfconfig.h sndfile.h sfendian.h common.h wav_w64.h
nist.c:		sfconfig.h sndfile.h sfendian.h common.h
ogg.c:		sfconfig.h sndfile.h sfendian.h common.h
paf.c:		sfconfig.h sndfile.h sfendian.h common.h
pcm.c:		sfconfig.h sndfile.h sfendian.h common.h
pvf.c:		sfconfig.h sndfile.h sfendian.h common.h
raw.c:		sfconfig.h sndfile.h common.h
rf64.c:		sfconfig.h sndfile.h sfendian.h common.h wav_w64.h
rx2.c:		sfconfig.h sndfile.h sfendian.h common.h
sd2.c:		sfconfig.h sndfile.h sfendian.h common.h
sds.c:		sfconfig.h sndfile.h sfendian.h common.h
sndfile.c:	sfconfig.h sndfile.h sfendian.h common.h
strings.c:	sfconfig.h sndfile.h common.h
svx.c:		sfconfig.h sndfile.h sfendian.h common.h
test_audio_detect.c:	sfconfig.h common.h sfendian.h test_main.h
test_broadcast_var.c:	sfconfig.h common.h test_main.h
test_conversions.c:	sfconfig.h common.h test_main.h
test_endswap.c:	sfconfig.h common.h sfendian.h test_main.h
test_file_io.c:	sfconfig.h common.h test_main.h
test_float.c:	sfconfig.h common.h test_main.h
test_ima_oki_adpcm.c:	sfconfig.h test_main.h ima_oki_adpcm.c
test_log_printf.c:	sfconfig.h common.h test_main.h
test_main.c:	sfconfig.h test_main.h
test_strncpy_crlf.c:	sfconfig.h common.h test_main.h
txw.c:		sfconfig.h sndfile.h sfendian.h common.h
ulaw.c:		sfconfig.h sndfile.h common.h
voc.c:		sfconfig.h sndfile.h sfendian.h common.h
vox_adpcm.c:	sfconfig.h sndfile.h sfendian.h common.h ima_oki_adpcm.h
w64.c:		sfconfig.h sndfile.h sfendian.h common.h wav_w64.h
wav.c:		sfconfig.h sndfile.h sfendian.h common.h wav_w64.h
wav_w64.c:	sfconfig.h sndfile.h sfendian.h common.h wav_w64.h
windows.c:	sfconfig.h sndfile.h common.h
wve.c:		sfconfig.h sndfile.h sfendian.h common.h
xi.c:		sfconfig.h sndfile.h sfendian.h common.h
sfconfig.h:	..\..\include\config.h
sndfile.h:	..\..\include\config.h
