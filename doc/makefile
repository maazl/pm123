#
#  pm123.inf makefile
#

TOPDIR = ..\src

!include $(TOPDIR)\config\makerules

all: pm123.inf pm123_pdk.inf $(MDUMMY)

MANLST = history.html manual\index.html manual\advantage.html manual\interface.html
MANLST = $(MANLST) manual\mainwin.html manual\mainmenu.html manual\playlist.html
MANLST = $(MANLST) manual\properties.html manual\id3.html manual\analyzer.html
MANLST = $(MANLST) manual\eq.html manual\manager.html manual\parameters.html
MANLST = $(MANLST) manual\dragndrop.html manual\skins.html manual\remote.html
MANLST = $(MANLST) manual\contact.html manual\copyright.html manual\netscape.html
MANLST = $(MANLST) manual\sguide.html manual\troubleshooting.html

PDKLST = pdk\index.html pdk\decode_plug.html pdk\filter_plug.html
PDKLST = $(PDKLST) pdk\output_plug.html pdk\visual_plug.html pdk\style.html

pm123.inf: pm123.ipf
	ipfc pm123.ipf /d:1 /c:850 $@
  
pm123.ipf: $(MANLST)
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- manual\index.html $@

pm123_pdk.inf: pm123_pdk.ipf
	ipfc pm123_pdk.ipf /d:1 /c:850 $@
  
pm123_pdk.ipf: $(PDKLST)
	set path=$(PATH);tools
	tools\html2ipf.cmd -co- -so- pdk\index.html $@

clean: $(MDUMMY)
	-@del *.inf 2> nul
	-@del *.ipf 2> nul
	-@del manual\images\*.bmp 2> nul
