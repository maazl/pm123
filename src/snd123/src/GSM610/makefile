#
#  gsm610 library makefile
#

DLL     = yes
TOPDIR  = ..\..\..

!include $(TOPDIR)\config\makerules

all: gsm610$(LBO) $(MDUMMY)

OBJECTS = add$(CO) decode$(CO) gsm_decode$(CO) gsm_encode$(CO) long_term$(CO)
OBJECTS = $(OBJECTS) preprocess$(CO) short_term$(CO) code$(CO) gsm_create$(CO)
OBJECTS = $(OBJECTS) gsm_destroy$(CO) gsm_option$(CO) lpc$(CO) rpe$(CO)
OBJECTS = $(OBJECTS) table$(CO)

gsm610$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del gsm610$(LBO) $(OBJECTS) 2> nul
