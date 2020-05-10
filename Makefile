#
#  makefile for the whole pm123 package
#

VERSION = 1.42b1
PARTS =	src\xio123\xio123.dll \
	src\fft123\fft123.dll \
	src\zlb123\zlb123.dll \
	src\plug-ins\analyzer\analyzer.dll \
	src\plug-ins\cddaplay\cddaplay.dll \
	src\plug-ins\mpg123\mpg123.dll \
	src\plug-ins\os2audio\os2audio.dll \
	src\plug-ins\oggplay\oggplay.dll \
	src\plug-ins\flac123\flac123.dll \
	src\plug-ins\realeq\realeq.dll \
	src\plug-ins\wavplay\wavplay.dll \
	src\plug-ins\wavout\wavout.dll \
	src\plug-ins\os2rec\os2rec.dll \
	src\plug-ins\pulse123\pulse123.dll \
	src\plug-ins\foldr123\foldr123.dll \
	src\plug-ins\plist123\plist123.dll \
	src\plug-ins\drc123\drc123.dll \
	src\plug-ins\aacplay\aacplay.dll \
	src\pm123\pm123.exe \
	src\skinutil\skinutil.exe \
	doc\pm123.inf \
	doc\pm123_pdk.inf

LIBPARTS= src\utils\utilfct$(LBO) src\utils\cpp\cpputil$(LBO) \
	src\gbm123\libgbm$(LBO) \
	src\snd123\src\sndfile$(LBO) src\libmpg123\libmpg123$(LBO) \
	src\ogg123\src\libogg$(LBO) src\vrb123\lib\libvorbis$(LBO) \
	src\libflac\src\libFLAC\libFLAC$(LBO) \
	src\pulseaudio\pulsecore\libpulsecore$(LBO) src\pulseaudio\pulse\libpulse$(LBO) \
	src\pulseaudio\json-c\libjson$(LBO) \
	src\libfaad\libfaad\libfaad$(LBO)

TARGET  = $(PARTS)

include src\config\makerules

all: $(TARGET)

src\utils\utilfct$(LBO):
	@$(MAKE) -C src\utils $(MFLAGS)

src\utils\cpp\cpputil$(LBO) src\utils\cpp\cpputil_plugin$(LBO):
	@$(MAKE) -C src\utils\cpp $(MFLAGS)

src\gbm123\libgbm$(LBO):
	@$(MAKE) -C src\gbm123 $(MFLAGS)

src\fft123\fft123.dll src\fft123\fft123$(LBI): src\utils\utilfct$(LBO)
	@$(MAKE) -C src\fft123 $(MFLAGS)

src\xio123\xio123.dll src\xio123\xio123$(LBI): src\utils\utilfct$(LBO) src\utils\cpp\cpputil$(LBO)
	@$(MAKE) -C src\xio123 $(MFLAGS)

src\libmpg123\libmpg123$(LBO):
	@$(MAKE) -C src\libmpg123 $(MFLAGS)

src\snd123\src\sndfile$(LBO):
	@$(MAKE) -C src\snd123 $(MFLAGS)

src\ogg123\src\libogg$(LBO):
	@$(MAKE) -C src\ogg123 $(MFLAGS)

src\vrb123\lib\libvorbis$(LBO): src\ogg123\src\libogg$(LBO)
	@$(MAKE) -C src\vrb123 $(MFLAGS)

src\libflac\src\libFLAC\libFLAC$(LBO):
	@$(MAKE) -C src\libflac\src\libFLAC $(MFLAGS)

src\zlb123\zlb123.dll src\zlb123\zlb123$(LBI):
	@$(MAKE) -C src\zlb123 $(MFLAGS)

src\pulseaudio\libspeex\libspeex$(LBO):
	@$(MAKE) -C src\pulseaudio\libspeex $(MFLAGS)

src\pulseaudio\json-c\libjson$(LBO):
	@$(MAKE) -C src\pulseaudio\json-c $(MFLAGS)

src\pulseaudio\pulsecore\libpulsecore$(LBO):
	@$(MAKE) -C src\pulseaudio\pulsecore $(MFLAGS)

src\pulseaudio\pulse\libpulse$(LBO):
	@$(MAKE) -C src\pulseaudio\pulse $(MFLAGS)

src\libfaad\libfaad\libfaad$(LBO):
	@$(MAKE) -C src\libfaad\libfaad $(MFLAGS)

src\libfaad\common\mp4ff\mp4ff$(LBO):
	@$(MAKE) -C src\libfaad\common\mp4ff $(MFLAGS)

src\plug-ins\analyzer\analyzer.dll: src\utils\utilfct$(LBO) src\fft123\fft123$(LBI)
	@$(MAKE) -C src\plug-ins\analyzer $(MFLAGS)

src\plug-ins\cddaplay\cddaplay.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI)
	@$(MAKE) -C src\plug-ins\cddaplay $(MFLAGS)

src\plug-ins\mpg123\mpg123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\zlb123\zlb123$(LBI) src\libmpg123\libmpg123$(LBO)
	@$(MAKE) -C src\plug-ins\mpg123 $(MFLAGS)

src\plug-ins\os2audio\os2audio.dll: src\utils\utilfct$(LBO)
	@$(MAKE) -C src\plug-ins\os2audio $(MFLAGS)

src\plug-ins\realeq\realeq.dll: src\utils\utilfct$(LBO) src\fft123\fft123$(LBI)
	@$(MAKE) -C src\plug-ins\realeq $(MFLAGS)

src\plug-ins\wavplay\wavplay.dll: src\utils\utilfct$(LBO) src\snd123\src\sndfile$(LBO) src\xio123\xio123$(LBI)
	@$(MAKE) -C src\plug-ins\wavplay $(MFLAGS)

src\plug-ins\wavout\wavout.dll: src\utils\utilfct$(LBO)
	@$(MAKE) -C src\plug-ins\wavout $(MFLAGS)

src\plug-ins\oggplay\oggplay.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\ogg123\src\libogg$(LBO) src\vrb123\lib\libvorbis$(LBO)
	@$(MAKE) -C src\plug-ins\oggplay $(MFLAGS)

src\plug-ins\flac123\flac123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\libflac\src\libFLAC\libFLAC$(LBO)
	@$(MAKE) -C src\plug-ins\flac123 $(MFLAGS)

src\plug-ins\os2rec\os2rec.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	@$(MAKE) -C src\plug-ins\os2rec $(MFLAGS)

src\plug-ins\pulse123\pulse123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\pulseaudio\pulsecore\libpulsecore$(LBO) src\pulseaudio\pulse\libpulse$(LBO) src\pulseaudio\json-c\libjson$(LBO)
	@$(MAKE) -C src\plug-ins\pulse123 $(MFLAGS)

src\plug-ins\foldr123\foldr123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	@$(MAKE) -C src\plug-ins\foldr123 $(MFLAGS)

src\plug-ins\plist123\plist123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	@$(MAKE) -C src\plug-ins\plist123 $(MFLAGS)

src\plug-ins\drc123\drc123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	@$(MAKE) -C src\plug-ins\drc123 $(MFLAGS)

src\plug-ins\aacplay\aacplay.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\zlb123\zlb123$(LBI) src\libfaad\libfaad\libfaad$(LBO) src\libfaad\common\mp4ff\mp4ff$(LBO)
	@$(MAKE) -C src\plug-ins\aacplay $(MFLAGS)

src\pm123\pm123.exe: src\utils\utilfct$(LBO) src\utils\cpp\cpputil$(LBO) src\xio123\xio123$(LBI) src\gbm123\libgbm$(LBO)
	@$(MAKE) -C src\pm123 $(MFLAGS)

src\skinutil\skinutil.exe: src\utils\utilfct$(LBO) src\gbm123\libgbm$(LBO)
	@$(MAKE) -C src\skinutil $(MFLAGS)

doc\pm123.inf doc\pm123_pdk.inf: $(MDUMMY)
	@$(MAKE) -C doc $(MFLAGS)

cleanparts: $(MDUMMY)
	-@del pm123.exe $(PARTS) $(LIBPARTS) 2> nul

clean:  clean123 $(MDUMMY)
	@$(MAKE) -C src\gbm123 $(MFLAGS) clean
	@$(MAKE) -C src\fft123 $(MFLAGS) clean
	@$(MAKE) -C src\libmpg123 $(MFLAGS) clean
	@$(MAKE) -C src\snd123 $(MFLAGS) clean
	@$(MAKE) -C src\ogg123 $(MFLAGS) clean
	@$(MAKE) -C src\vrb123 $(MFLAGS) clean
	@$(MAKE) -C src\libflac\src\libFLAC $(MFLAGS) clean
	@$(MAKE) -C src\pulseaudio\libspeex $(MFLAGS) clean
	@$(MAKE) -C src\pulseaudio\json-c $(MFLAGS) clean
	@$(MAKE) -C src\pulseaudio\pulsecore $(MFLAGS) clean
	@$(MAKE) -C src\pulseaudio\pulse $(MFLAGS) clean
	@$(MAKE) -C src\zlb123 $(MFLAGS) clean
	@$(MAKE) -C doc $(MFLAGS) clean

clean123: $(MDUMMY)
	@$(MAKE) -C src\utils $(MFLAGS) clean
	@$(MAKE) -C src\utils\cpp $(MFLAGS) clean
	@$(MAKE) -C src\xio123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\analyzer $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\cddaplay $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\mpg123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\os2audio $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\os2rec $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\realeq $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\wavplay $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\wavout $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\oggplay $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\flac123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\os2rec $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\pulse123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\foldr123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\plist123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\drc123 $(MFLAGS) clean
	@$(MAKE) -C src\plug-ins\aacplay $(MFLAGS) clean
	@$(MAKE) -C src\pm123 $(MFLAGS) clean
	@$(MAKE) -C doc $(MFLAGS) clean

depend: $(MDUMMY)
	@$(MAKE) -C src\utils $(MFLAGS) depend
	@$(MAKE) -C src\utils\cpp $(MFLAGS) depend
	@$(MAKE) -C src\gbm123 $(MFLAGS) depend
	@$(MAKE) -C src\xio123 $(MFLAGS) depend
	@$(MAKE) -C src\libmpg123 $(MFLAGS) depend
	@$(MAKE) -C src\snd123 $(MFLAGS) depend
	@$(MAKE) -C src\ogg123 $(MFLAGS) depend
	@$(MAKE) -C src\vrb123 $(MFLAGS) depend
	@$(MAKE) -C src\libflac\src\libFLAC $(MFLAGS) depend
	@$(MAKE) -C src\pulseaudio\libspeex $(MFLAGS) depend
	@$(MAKE) -C src\pulseaudio\json-c $(MFLAGS) depend
	@$(MAKE) -C src\pulseaudio\pulsecore $(MFLAGS) depend
	@$(MAKE) -C src\pulseaudio\pulse $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\analyzer $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\cddaplay $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\mpg123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\realeq $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\wavplay $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\wavout $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\oggplay $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\flac123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\os2audio $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\os2rec $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\pulse123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\foldr123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\plist123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\drc123 $(MFLAGS) depend
	@$(MAKE) -C src\plug-ins\aacplay $(MFLAGS) depend
	@$(MAKE) -C src\pm123 $(MFLAGS) depend
	@$(MAKE) -C src\skinutil $(MFLAGS) depend

dist: distzip distpackage $(MDUMMY)

distfiles: filesclean $(PARTS) $(MDUMMY)
	-@mkdir dist\files         2>nul
	-@mkdir dist\files\visplug 2>nul
	-@mkdir dist\files\icons   2>nul
	-@mkdir dist\files\pdk     2>nul
	copy src\fft123\fft123.dll      dist\files
	copy src\xio123\xio123.dll      dist\files
	copy src\zlb123\zlb123.dll      dist\files
	copy src\plug-ins\analyzer\analyzer.dll dist\files\visplug
	copy src\plug-ins\cddaplay\cddaplay.dll dist\files
	copy src\plug-ins\mpg123\mpg123.dll     dist\files
	copy src\plug-ins\os2audio\os2audio.dll dist\files
	copy src\plug-ins\realeq\realeq.dll     dist\files
	copy src\plug-ins\wavout\wavout.dll     dist\files
	copy src\plug-ins\wavplay\wavplay.dll   dist\files
	copy src\plug-ins\oggplay\oggplay.dll   dist\files
	copy src\plug-ins\flac123\flac123.dll   dist\files
	copy src\plug-ins\os2rec\os2rec.dll     dist\files
	copy src\plug-ins\pulse123\pulse123.dll dist\files
	copy src\plug-ins\foldr123\foldr123.dll dist\files
	copy src\plug-ins\plist123\plist123.dll dist\files
	copy src\plug-ins\drc123\drc123.dll     dist\files
	copy src\plug-ins\aacplay\aacplay.dll     dist\files
	copy src\pm123\pm123.exe        dist\files
	copy src\pm123\default.skn      dist\files
	copy src\skinutil\skinutil.exe  dist\files
	copy doc\common\history.html    dist\files
	copy doc\pm123.inf              dist\files
	copy doc\pm123_pdk.inf          dist\files\pdk
	copy src\WPS\icons\*.ico        dist\files\icons
	copy src\WPS\makewps.cmd        dist\files
	copy src\include\*.h            dist\files\pdk
	copy src\fft123\fft123.lib      dist\files\pdk
	copy src\fft123\api\fftw3.h     dist\files\pdk\fft123.h
	copy src\fft123\doc\fftw3.pdf   dist\files\pdk\fft123.pdf
	copy src\xio123\xio123.lib      dist\files\pdk\xio123.lib
	copy src\xio123\xio.h           dist\files\pdk\xio123.h
	copy COPYING.html               dist\files
	copy COPYRIGHT.html             dist\files

distpackage: distfiles $(MDUMMY)
	if exist dist\pm123-$(VERSION).exe del dist\pm123-$(VERSION).exe
	wic.exe -a dist\pm123-$(VERSION).exe 1 -U -r -cdist\files * -s dist\warpin.wis
	if exist dist\pm123-$(VERSION).wpi del dist\pm123-$(VERSION).wpi
	wic.exe -a dist\pm123-$(VERSION).wpi 1 -r -cdist\files * -s dist\warpin.wis

distzip: distfiles $(MDUMMY)
	if exist dist\pm123-$(VERSION).zip del dist\pm123-$(VERSION).zip
ifdef DEBUG_LOG
	cmd /c "cd dist\files & zip -rX ..\pm123-$(VERSION)-debug.zip * -x .svn CVS\* .cvsignore "
else
	cmd /c "cd dist\files & zip -rX ..\pm123-$(VERSION).zip * -x .svn CVS\* .cvsignore "
endif

distsrc: $(MDUMMY)
	-@del dist\pm123-$(VERSION)-src.zip  2> nul
	zip -9 -r dist\pm123-$(VERSION)-src.zip * -x .svn .git *.zip dist\files\* src\config\makerules *$(CO) *$(LBI) *$(LBO) *.res *.dll *.exe doc\*.bmp *.ipf *.inf doxydoc\* diff idx stat stat2

distclean: filesclean $(MDUMMY)
	-@del dist\pm123-$(VERSION).exe 2> nul
	-@del dist\pm123-$(VERSION).zip 2> nul
	-@del dist\pm123-$(VERSION)-src.zip 2> nul
	-@del dist\pm123-$(VERSION).wpi 2> nul

filesclean: $(MDUMMY)
	-@echo Cleanups...
	-@del dist\files\icons\* /n     2> nul
	-@rd  dist\files\icons          2> nul
	-@del dist\files\visplug\* /n   2> nul
	-@rd  dist\files\visplug        2> nul
	-@del dist\files\pdk\* /n       2> nul
	-@rd  dist\files\pdk            2> nul
	-@del dist\files\cddb\blues\* /n      2> nul
	-@rd  dist\files\cddb\blues           2> nul
	-@del dist\files\cddb\classical\* /n  2> nul
	-@rd  dist\files\cddb\classical       2> nul
	-@del dist\files\cddb\country\* /n    2> nul
	-@rd  dist\files\cddb\country         2> nul
	-@del dist\files\cddb\data\* /n       2> nul
	-@rd  dist\files\cddb\data            2> nul
	-@del dist\files\cddb\folk\* /n       2> nul
	-@rd  dist\files\cddb\folk            2> nul
	-@del dist\files\cddb\jazz\* /n       2> nul
	-@rd  dist\files\cddb\jazz            2> nul
	-@del dist\files\cddb\newage\* /n     2> nul
	-@rd  dist\files\cddb\newage          2> nul
	-@del dist\files\cddb\reggae\* /n     2> nul
	-@rd  dist\files\cddb\reggae          2> nul
	-@del dist\files\cddb\rock\* /n       2> nul
	-@rd  dist\files\cddb\rock            2> nul
	-@del dist\files\cddb\soundtrack\* /n 2> nul
	-@rd  dist\files\cddb\soundtrack      2> nul
	-@del dist\files\cddb\misc\* /n       2> nul
	-@rd  dist\files\cddb\misc            2> nul
	-@rd  dist\files\cddb                 2> nul
	-@del dist\files\COPYING        2> nul
	-@del dist\files\COPYRIGHT      2> nul
	-@del dist\files\*.cmd          2> nul
	-@del dist\files\*.dll          2> nul
	-@del dist\files\*.exe          2> nul
	-@del dist\files\*.skn          2> nul
	-@del dist\files\*.inf          2> nul
	-@del dist\files\*.txt          2> nul
	-@del dist\files\*.htm          2> nul
	-@del dist\files\*.html         2> nul
	-@del dist\files\*.ini          2> nul
	-@del dist\files\*.lst          2> nul
	-@del dist\files\*.bak          2> nul
	-@del dist\files\*.mgr          2> nul
	-@del dist\files\*.log          2> nul

test: distfiles
	@$(MAKE) -C test $(MFLAGS)
