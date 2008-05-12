#
#  pm123.inf makefile
#

TOPDIR = ..\src
TARGET = pm123.inf pm123_pdk.inf

!include $(TOPDIR)\config\makerules

all: pm123.inf pm123_pdk.inf $(MDUMMY)

MANLST = history.html manual\index.html manual\advantage.html
MANLST = $(MANLST) manual\mainwin.html manual\mainmenu.html manual\playlist.html
MANLST = $(MANLST) manual\properties.html manual\id3.html manual\analyzer.html
MANLST = $(MANLST) manual\eq.html manual\manager.html manual\parameters.html
MANLST = $(MANLST) manual\playlistformat.html manual\skins.html manual\remote.html
MANLST = $(MANLST) manual\contact.html manual\copyright.html manual\netscape.html
MANLST = $(MANLST) manual\sguide.html manual\troubleshooting.html

PDKLST = pdk\index.html pdk\decode_plug.html pdk\filter_plug.html
PDKLST = $(PDKLST) pdk\output_plug.html pdk\visual_plug.html

pm123.inf: pm123.ipf
	ipfc pm123.ipf /d:1 /c:850 $@

pm123.ipf: $(MANLST) tools\html2ipf.cmd
	setlocal
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- -p+ manual\index.html $@
	endlocal

pm123_pdk.inf: pm123_pdk.ipf
	ipfc pm123_pdk.ipf /d:1 /c:850 $@

pm123_pdk.ipf: $(PDKLST) tools\html2ipf.cmd
	setlocal
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- -p+ pdk\index.html $@
	endlocal

clean: $(MDUMMY)
	-@del *.inf 2> nul
	-@del *.ipf 2> nul
	-@del manual\images\*.bmp 2> nul
