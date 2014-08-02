#
#  makefile for the whole pm123 package
#

VERSION = 1.41b1
PARTS   = $(PARTS) src\xio123\xio123.dll
PARTS   = $(PARTS) src\fft123\fft123.dll
PARTS   = $(PARTS) src\zlb123\zlb123.dll
PARTS   = $(PARTS) src\plug-ins\analyzer\analyzer.dll
PARTS   = $(PARTS) src\plug-ins\cddaplay\cddaplay.dll
PARTS   = $(PARTS) src\plug-ins\mpg123\mpg123.dll
PARTS   = $(PARTS) src\plug-ins\os2audio\os2audio.dll
PARTS   = $(PARTS) src\plug-ins\oggplay\oggplay.dll
PARTS   = $(PARTS) src\plug-ins\flac123\flac123.dll
PARTS   = $(PARTS) src\plug-ins\realeq\realeq.dll
PARTS   = $(PARTS) src\plug-ins\wavplay\wavplay.dll
PARTS   = $(PARTS) src\plug-ins\wavout\wavout.dll
PARTS   = $(PARTS) src\plug-ins\os2rec\os2rec.dll
PARTS   = $(PARTS) src\plug-ins\pulse123\pulse123.dll
PARTS   = $(PARTS) src\plug-ins\foldr123\foldr123.dll
PARTS   = $(PARTS) src\plug-ins\plist123\plist123.dll
PARTS   = $(PARTS) src\plug-ins\drc123\drc123.dll
PARTS   = $(PARTS) src\pm123\pm123.exe
PARTS   = $(PARTS) src\skinutil\skinutil.exe
PARTS   = $(PARTS) doc\pm123.inf

TARGET  = $(PARTS)

!include src\config\makerules

all: $(TARGET)

src\utils\utilfct$(LBO):
	cd src\utils
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\utils\cpp\cpputil$(LBO) src\utils\cpp\cpputil_plugin$(LBO):
	cd src\utils\cpp
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\gbm123\libgbm$(LBO):
	cd src\gbm123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\fft123\fft123.dll src\fft123\fft123$(LBI): src\utils\utilfct$(LBO)
	cd src\fft123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\xio123\xio123.dll src\xio123\xio123$(LBI): src\utils\utilfct$(LBO) src\utils\cpp\cpputil$(LBO)
	cd src\xio123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\libmpg123\src\libmpg123$(LBO):
	cd src\libmpg123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\snd123\src\sndfile$(LBO):
	cd src\snd123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\ogg123\src\libogg$(LBO):
	cd src\ogg123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\vrb123\lib\libvorbis$(LBO): src\ogg123\src\libogg$(LBO)
	cd src\vrb123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\libflac\src\libFLAC\libFLAC$(LBO):
	cd src\libflac\src\libFLAC
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..\..

src\zlb123\zlb123.dll src\zlb123\zlb123$(LBI):
	cd src\zlb123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\pulseaudio\libspeex\libspeex$(LBO):
	cd src\pulseaudio\libspeex
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\pulseaudio\json-c\libjson$(LBO):
	cd src\pulseaudio\json-c
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\pulseaudio\pulsecore\pulsecore$(LBO):
	cd src\pulseaudio\pulsecore
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\pulseaudio\pulse\pulse$(LBO):
	cd src\pulseaudio\pulse
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\analyzer\analyzer.dll: src\utils\utilfct$(LBO) src\fft123\fft123$(LBI)
	cd src\plug-ins\analyzer
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\cddaplay\cddaplay.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI)
	cd src\plug-ins\cddaplay
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\mpg123\mpg123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\zlb123\zlb123$(LBI) src\libmpg123\src\libmpg123$(LBO)
	cd src\plug-ins\mpg123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\os2audio\os2audio.dll: src\utils\utilfct$(LBO)
	cd src\plug-ins\os2audio
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\realeq\realeq.dll: src\utils\utilfct$(LBO) src\fft123\fft123$(LBI)
	cd src\plug-ins\realeq
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\wavplay\wavplay.dll: src\utils\utilfct$(LBO) src\snd123\src\sndfile$(LBO) src\xio123\xio123$(LBI)
	cd src\plug-ins\wavplay
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\wavout\wavout.dll: src\utils\utilfct$(LBO)
	cd src\plug-ins\wavout
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\oggplay\oggplay.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\ogg123\src\libogg$(LBO) src\vrb123\lib\libvorbis$(LBO)
	cd src\plug-ins\oggplay
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\flac123\flac123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\xio123\xio123$(LBI) src\libflac\src\libFLAC\libFLAC$(LBO)
	cd src\plug-ins\flac123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\os2rec\os2rec.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	cd src\plug-ins\os2rec
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\pulse123\pulse123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO) src\pulseaudio\pulsecore\pulsecore$(LBO) src\pulseaudio\pulse\pulse$(LBO) src\pulseaudio\json-c\libjson$(LBO)
	cd src\plug-ins\pulse123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\foldr123\foldr123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	cd src\plug-ins\foldr123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\plist123\plist123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	cd src\plug-ins\plist123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\plug-ins\drc123\drc123.dll: src\utils\utilfct$(LBO) src\utils\cpp\cpputil_plugin$(LBO)
	cd src\plug-ins\drc123
	@$(MAKE) $(MFLAGS)
	@cd ..\..\..

src\pm123\pm123.exe: src\utils\utilfct$(LBO) src\utils\cpp\cpputil$(LBO) src\xio123\xio123$(LBI) src\gbm123\libgbm$(LBO)
	cd src\pm123
	@$(MAKE) $(MFLAGS)
	@cd ..\..

src\skinutil\skinutil.exe: src\utils\utilfct$(LBO) src\gbm123\libgbm$(LBO)
	cd src\skinutil
	@$(MAKE) $(MFLAGS)
	@cd ..\..

doc\pm123.inf:
	cd doc
	@$(MAKE) $(MFLAGS)
	@cd ..

clean:  clean123 $(MDUMMY)
	cd src\gbm123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\fft123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\libmpg123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\snd123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\ogg123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\vrb123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\pulseaudio\libspeex
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\pulseaudio\json-c
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\pulseaudio\pulsecore
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\pulseaudio\pulse
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\zlb123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd doc
	@$(MAKE) $(MFLAGS) clean
	@cd ..

clean123: $(MDUMMY)
	cd src\utils
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\utils\cpp
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\xio123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd src\plug-ins\analyzer
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\cddaplay
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\mpg123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\os2audio
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\os2rec
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\realeq
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\wavplay
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\wavout
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\oggplay
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\flac123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\os2rec
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\pulse123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\foldr123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\plist123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\plug-ins\drc123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..\..
	cd src\pm123
	@$(MAKE) $(MFLAGS) clean
	@cd ..\..
	cd doc
	@$(MAKE) $(MFLAGS) clean
	@cd ..

depend: $(MDUMMY)
	cd src\utils
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\utils\cpp
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\gbm123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\xio123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\libmpg123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\snd123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\ogg123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\vrb123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\pulseaudio\libspeex
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\pulseaudio\json-c
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\pulseaudio\pulsecore
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\pulseaudio\pulse
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\analyzer
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\cddaplay
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\mpg123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\realeq
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\wavplay
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\wavout
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\oggplay
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\flac123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\os2audio
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\os2rec
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\pulse123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\foldr123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\plist123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\plug-ins\drc123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..\..
	cd src\pm123
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\main
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..
	cd src\skinutil
	@$(MAKE) $(MFLAGS) depend
	@cd ..\..

dist: distzip distpackage $(MDUMMY)

distfiles: filesclean $(PARTS) $(MDUMMY)
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
!ifdef DEBUG_LOG
	cmd /c "cd dist\files & zip -rX ..\pm123-$(VERSION)-debug.zip * -x .svn CVS\* .cvsignore "
!else
	cmd /c "cd dist\files & zip -rX ..\pm123-$(VERSION).zip * -x .svn CVS\* .cvsignore "
!endif

distsrc: $(MDUMMY)
	-@del dist\pm123-$(VERSION)-src.zip  2> nul
	zip -9 -r dist\pm123-$(VERSION)-src.zip * -x .svn *.zip dist\files\* src\config\makerules *$(CO) *$(LBI) *$(LBO) *.res *.dll *.exe doc\*.bmp *.ipf *.inf doxydoc\* diff idx stat stat2

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
	cd test
	@$(MAKE) $(MFLAGS)
	@cd ..

