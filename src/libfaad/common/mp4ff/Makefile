#
#  g72x library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
TARGET  = all2
CFLAGS_USR = -DUSE_TAGGING=1

include $(TOPDIR)\config\makerules

all2: mp4ff$(LBO) $(MDUMMY)

OBJECTS = mp4ff$(CO) mp4atom$(CO) mp4meta$(CO) mp4sample$(CO) mp4util$(CO) mp4tagupdate$(CO)

mp4ff$(LBO): $(LIBS) $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del mp4ff$(LBO) $(OBJECTS) 2> nul
