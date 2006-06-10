#
#  makefile for the whole pm123 package using NMAKE and VAC++
#

VERSION = 1_32

!include src\config\makerules

!IFDEF DEBUG
DBGFLAG = $(DBGFLAG) DEBUG=Yes
!ENDIF
!IFDEF DEBUG_ALLOC
DBGFLAG = $(DBGFLAG) DEBUG_ALLOC=Yes
!ENDIF

MAKEME  = $(MAKE) $(MKFLAG) $(DBGFLAG)

all: utilfct.lib gbm123.dll http123.dll ooura1d.dll \
     analyzer.dll fft123.dll cddaplay.dll mpg123.dll os2audio.dll realeq.dll \
     wavout.dll wavplay.dll pm123.exe skinutil.exe pm123.inf

utilfct.lib:
   cd src\utils
   @$(MAKEME)
   @cd ..\..

gbm123.dll:
   cd src\gbm123
   @$(MAKEME)
   @cd ..\..

fft123.dll:
   cd src\fft123
   @$(MAKEME)
   @cd ..\..

http123.dll:
   cd src\http123
   @$(MAKEME)
   @cd ..\..

ooura1d.dll:
   cd src\ooura1d
   @$(MAKEME)
   @cd ..\..

analyzer.dll:
   cd src\plug-ins\analyzer
   @$(MAKEME)
   @cd ..\..\..

cddaplay.dll:
   cd src\plug-ins\cddaplay
   @$(MAKEME)
   @cd ..\..\..

mpg123.dll:
   cd src\plug-ins\mpg123
   @$(MAKEME)
   @cd ..\..\..

os2audio.dll:
   cd src\plug-ins\os2audio
   @$(MAKEME)
   @cd ..\..\..

realeq.dll:
   cd src\plug-ins\realeq
   @$(MAKEME)
   @cd ..\..\..

wavout.dll:
   cd src\plug-ins\wavout
   @$(MAKEME)
   @cd ..\..\..

wavplay.dll:
   cd src\plug-ins\wavplay
   @$(MAKEME)
   @cd ..\..\..

pm123.exe:
   cd src\pm123
   @$(MAKEME)
   @cd ..\..

skinutil.exe:
   cd src\skinutil
   @$(MAKEME)
   @cd ..\..

pm123.inf:
   cd doc
   @$(MAKEME)
   @cd ..

clean:
   cd src\utils
   @$(MAKEME) clean
   @cd ..\..
   cd src\gbm123
   @$(MAKEME) clean
   @cd ..\..
   cd src\fft123
   @$(MAKEME) clean
   @cd ..\..
   cd src\http123
   @$(MAKEME) clean
   @cd ..\..
   cd src\ooura1d
   @$(MAKEME) clean
   @cd ..\..
   cd src\plug-ins\analyzer
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\cddaplay
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\mpg123
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\os2audio
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\realeq
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\wavout
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\plug-ins\wavplay
   @$(MAKEME) clean
   @cd ..\..\..
   cd src\pm123
   @$(MAKEME) clean
   @cd ..\..
   cd src\skinutil
   @$(MAKEME) clean
   @cd ..\..
   cd doc
   @$(MAKEME) clean
   @cd ..

dist: distfiles distpackage distzip

distfiles: 
   if exist dist\files\visplug\* del dist\files\visplug\* /n
   if exist dist\files\visplug rd dist\files\visplug
   if exist dist\files\icons\* del dist\files\icons\* /n
   if exist dist\files\icons rd dist\files\icons
   if exist dist\files\pdk\* del dist\files\pdk\* /n
   if exist dist\files\pdk rd dist\files\pdk
   if exist dist\files\* for %%f in (dist\files\*) do @if not %%f==dist\files\.cvsignore del %%f
   if not exist dist\files mkdir dist\files
   mkdir dist\files\visplug
   mkdir dist\files\icons
   mkdir dist\files\pdk
   copy src\gbm123\gbm123.dll dist\files
   copy src\fft123\fft123.dll dist\files
   copy src\http123\http123.dll dist\files
   copy src\ooura1d\ooura1d.dll dist\files
   copy src\plug-ins\analyzer\analyzer.dll dist\files\visplug
   copy src\plug-ins\cddaplay\cddaplay.dll dist\files
   copy src\plug-ins\mpg123\mpg123.dll dist\files
   copy src\plug-ins\os2audio\os2audio.dll dist\files
   copy src\plug-ins\realeq\realeq.dll dist\files
   copy src\plug-ins\wavout\wavout.dll dist\files
   copy src\plug-ins\wavplay\wavplay.dll dist\files
   copy src\pm123\pm123.exe dist\files
   copy src\pm123\default.skn dist\files
   copy src\skinutil\skinutil.exe dist\files
   copy doc\history.txt dist\files
   copy doc\pm123.txt dist\files
   copy doc\pm123.inf dist\files
   copy doc\pm123_pdk.inf dist\files\pdk
   copy src\WPS\icons\*.ico dist\files\icons
   copy src\WPS\makewps.cmd dist\files
   copy src\include\*.h dist\files\pdk
   copy src\fft123\fft123.lib dist\files\pdk
   copy src\fft123\api\fftw3.h dist\files\pdk\fft123.h
   copy src\fft123\doc\fftw3.pdf dist\files\pdk\fft123.pdf
   copy COPYING dist\files
   copy COPYRIGHT dist\files
   copy extra\irc\querysong.c dist\files

distpackage:
   if exist dist\pm123-$(VERSION).exe del dist\pm123-$(VERSION).exe
   wic.exe -a dist\pm123-$(VERSION).exe 1 -U -r -cdist\files * -s dist\warpin.wis
   if exist dist\pm123-$(VERSION).wpi del dist\pm123-$(VERSION).wpi
   wic.exe -a dist\pm123-$(VERSION).wpi 1 -r -cdist\files * -s dist\warpin.wis

distzip:
   if exist dist\pm123-$(VERSION).zip del dist\pm123-$(VERSION).zip
   cmd /c "cd dist\files & zip -rX ..\pm123-$(VERSION).zip * -x CVS\* .cvsignore "

distclean:
   if exist dist\files\visplug\* del dist\files\visplug\* /n
   if exist dist\files\visplug rd dist\files\visplug
   if exist dist\files\icons\* del dist\files\icons\* /n
   if exist dist\files\icons rd dist\files\icons
   if exist dist\files\pdk\* del dist\files\pdk\* /n
   if exist dist\files\pdk rd dist\files\pdk
   if exist dist\files\* for %%f in (dist\files\*) do @if not %%f==dist\files\.cvsignore del %%f
   if exist dist\pm123-$(VERSION).exe del dist\pm123-$(VERSION).exe
   if exist dist\pm123-$(VERSION).zip del dist\pm123-$(VERSION).zip
   if exist dist\pm123-$(VERSION).wpi del dist\pm123-$(VERSION).wpi
