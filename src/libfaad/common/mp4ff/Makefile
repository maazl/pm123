TOPDIR  = ..\..\..
TARGET  = all2
INCDIR  = .
CFLAGS_USR = -DUSE_TAGGING=1

include $(TOPDIR)\config\makerules

all2: mp4ff$(LBO) $(MDUMMY)

OBJECTS = mp4ff$(CO) mp4atom$(CO) mp4meta$(CO) mp4sample$(CO) mp4util$(CO) mp4tagupdate$(CO)

mp4ff$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del mp4ff$(LBO) $(OBJECTS) 2> nul
mp4atom.o:	mp4atom.c mp4ffint.h ..\..\config.h 
mp4ff.o:	mp4ff.c 
mp4meta.o:	mp4meta.c 
mp4sample.o:	mp4sample.c 
mp4tagupdate.o:	mp4tagupdate.c 
mp4util.o:	mp4util.c 
