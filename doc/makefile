#
#  pm123.inf makefile for NMAKE and VAC++
#

MAKEME = $(MAKE) /NOLOGO /$(MAKEFLAGS) $(DBGFLAG)

MANLST = history.txt            \
         manual\index.html      \
         manual\advantage.html  \
         manual\interface.html  \
         manual\mainwin.html    \
         manual\mainmenu.html   \
         manual\playlist.html   \
         manual\properties.html \
         manual\id3.html        \
         manual\analyzer.html   \
         manual\eq.html         \
         manual\manager.html    \
         manual\parameters.html \
         manual\dragndrop.html  \
         manual\skins.html      \
         manual\remote.html     \
         manual\contact.html    \
         manual\copyright.html  \
         manual\netscape.html   \
         manual\sguide.html     \
         manual\troubleshooting.html

PDKLST = pdk\index.html         \
         pdk\decode_plug.html   \
         pdk\filter_plug.html   \
         pdk\output_plug.html   \
         pdk\visual_plug.html 

all: pm123.inf \
     pm123_pdk.inf
     
pm123.inf: pm123.ipf
  ipfc $** /d:1 /c:850 $@
  
pm123.ipf: $(MANLST)
  set path=$(PATH);tools 
  tools\html2ipf.cmd -co- -so- manual\index.html $@

pm123_pdk.inf: pm123_pdk.ipf
  ipfc $** /d:1 /c:850 $@
  
pm123_pdk.ipf: $(PDKLST)
  set path=$(PATH);tools 
  tools\html2ipf.cmd -co- -so- pdk\index.html $@

clean:
  -@del *.inf 2> nul
  -@del *.ipf 2> nul
  -@del manual\images\*.bmp 2> nul
