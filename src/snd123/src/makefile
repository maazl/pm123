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
OBJECTS = $(OBJECTS) endswap$(CO)

sndfile$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del sndfile$(LBO) 2> nul
	-@del $(OBJECTS)    2> nul
