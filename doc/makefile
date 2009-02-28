#
#  pm123.inf makefile
#

TOPDIR = ..\src
TARGET = pm123.inf pm123_pdk.inf

!include $(TOPDIR)\config\makerules

all: pm123.inf pm123_pdk.inf $(MDUMMY)

MANLST = history.html manual\analyzer.html manual\analyzerformat.html manual\contact.html 
MANLST = $(MANLST) manual\copyright.html manual\display.html manual\dragndrop.html 
MANLST = $(MANLST) manual\gui.html manual\infodialog.html manual\id3edit.html
MANLST = $(MANLST) manual\index.html manual\integration.html 
MANLST = $(MANLST) manual\keyboard.html manual\location.html manual\mainmenu.html
MANLST = $(MANLST) manual\mainwin.html manual\manager.html manual\migrating.html 
MANLST = $(MANLST) manual\netscape.html manual\parameters.html manual\playlist.html 
MANLST = $(MANLST) manual\playlistformat.html manual\plug-ins.html manual\logvolume.html
MANLST = $(MANLST) manual\plug-ins_dec-cfg.html manual\plug-ins_fil-cfg.html 
MANLST = $(MANLST) manual\plug-ins_out-cfg.html manual\plug-ins_vis-cfg.html 
MANLST = $(MANLST) manual\properties.html manual\realeq.html manual\remote.html 
MANLST = $(MANLST) manual\remoterefconfig.html manual\remotereferror.html manual\remoterefgui.html
MANLST = $(MANLST) manual\remoterefplayback.html manual\remoterefplaylist.html
MANLST = $(MANLST) manual\settings1.html manual\settings2.html manual\sguide.html 
MANLST = $(MANLST) manual\skins.html manual\tech.html manual\troubleshooting.html 
MANLST = $(MANLST) manual\urlformat.html manual\bookmark.html manual\playlist_cm.html 
MANLST = $(MANLST) manual\playlist_cmi.html manual\os2rec.html manual\bugreport.html
MANLST = $(MANLST) manual\logfiles.html manual\opentodo.html manual\mpg123.html


PDKLST = pdk\index.html pdk\decode_plug.html pdk\filter_plug.html
PDKLST = $(PDKLST) pdk\output_plug.html pdk\visual_plug.html

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

manual\opentodo.html: todo.html
	tools\extracttodo.cmd <todo.html >$@

clean: $(MDUMMY)
	-@del *.inf 2> nul
	-@del *.ipf 2> nul
	-@del manual\images\*.bmp 2> nul
