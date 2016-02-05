#
#  pm123.inf makefile
#

TOPDIR = ..\src
TARGET = pm123.inf pm123_pdk.inf

!include $(TOPDIR)\config\makerules

all: pm123.inf pm123_pdk.inf $(MDUMMY)

MANLST = common\history.html common\todo.html
MANLST = $(MANLST) manual\analyzer.html manual\analyzerformat.html manual\contact.html
MANLST = $(MANLST) manual\copyright.html manual\display.html manual\dragndrop.html
MANLST = $(MANLST) manual\gui.html manual\infodialog.html manual\id3edit.html
MANLST = $(MANLST) manual\index.html manual\integration.html
MANLST = $(MANLST) manual\keyboard.html manual\location.html manual\mainmenu.html
MANLST = $(MANLST) manual\mainwin.html manual\manager.html manual\migrating.html
MANLST = $(MANLST) manual\netscape.html manual\parameters.html manual\playlist.html
MANLST = $(MANLST) manual\playlistformat.html manual\plug-ins.html
MANLST = $(MANLST) manual\plug-ins_dec-cfg.html manual\plug-ins_fil-cfg.html
MANLST = $(MANLST) manual\plug-ins_out-cfg.html manual\plug-ins_vis-cfg.html
MANLST = $(MANLST) manual\properties.html manual\realeq\index.html manual\remote.html
MANLST = $(MANLST) manual\drc123\index.html manual\drc123\configuration.html
MANLST = $(MANLST) manual\drc123\calibration.html manual\drc123\calibrationext.html
MANLST = $(MANLST) manual\drc123\measurement.html manual\drc123\measurementext.html
MANLST = $(MANLST) manual\drc123\generate.html manual\drc123\generateext.html
MANLST = $(MANLST) manual\drc123\deconvolution.html manual\drc123\deconvolutionext.html
MANLST = $(MANLST) manual\drc123\fileformat.html manual\drc123\graphs.html
MANLST = $(MANLST) manual\drc123\microphone.html manual\drc123\reference.html
MANLST = $(MANLST) manual\remote\index.html manual\remote\config.html manual\remote\error.html
MANLST = $(MANLST) manual\remote\gui.html manual\remote\meta.html manual\remote\metawrite.html
MANLST = $(MANLST) manual\remote\options.html manual\remote\playback.html manual\remote\playlist.html
MANLST = $(MANLST) manual\remote\plugin.html manual\remote\general.html
MANLST = $(MANLST) manual\settings1.html manual\settings2.html manual\setting_play.html 
MANLST = $(MANLST) manual\settingsio.html manual\sguide.html
MANLST = $(MANLST) manual\skins.html manual\tech.html manual\troubleshooting.html
MANLST = $(MANLST) manual\urlformat.html manual\bookmark.html manual\playlist_cm.html
MANLST = $(MANLST) manual\playlist_cmi.html manual\os2rec.html manual\bugreport.html
MANLST = $(MANLST) manual\logfiles.html manual\opentodo.html manual\mpg123.html
MANLST = $(MANLST) manual\os2audio.html manual\logvolume.html

PDKLST = pdk\index.html pdk\decoder_plug.html pdk\decoder_plug_1.html pdk\filter_plug.html
PDKLST = $(PDKLST) pdk\output_plug.html pdk\visual_plug.html
PDKLST = $(PDKLST) pdk\datatype.html pdk\xstring_api.html


pm123.inf: pm123.ipf
	ipfc pm123.ipf /d:1 /c:850 $@

pm123.ipf: $(MANLST)
	setlocal
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- -p+ manual\index.html $@
	endlocal

pm123_pdk.inf: pm123_pdk.ipf
	ipfc pm123_pdk.ipf /d:1 /c:850 $@

pm123_pdk.ipf: $(PDKLST)
	setlocal
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- -p+ pdk\index.html $@
	endlocal

manual\opentodo.html: common\todo.html
	tools\extracttodo.cmd <common\todo.html >$@

clean: $(MDUMMY)
	-@del *.inf 2> nul
	-@del *.ipf 2> nul
	-@del manual\images\*.bmp 2> nul

check.log: linkchecker-out.csv
	perl tools\parselinkchecker.pl linkchecker-out.csv >$@

linkchecker-out.csv: $(MANLST) $(PDKLST)
	linkchecker -Ftools\linkcheckerrc manual\index.html pdk\index.html developer\index.html common\todo.html >$@
