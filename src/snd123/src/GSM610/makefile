#
#  gsm610 library makefile
#

DLL     = yes
TOPDIR  = ..\..\..
TARGET  = all2

!include $(TOPDIR)\config\makerules

all2: gsm610$(LBO) $(MDUMMY)

OBJECTS = add$(CO) decode$(CO) gsm_decode$(CO) gsm_encode$(CO) long_term$(CO)
OBJECTS = $(OBJECTS) preprocess$(CO) short_term$(CO) code$(CO) gsm_create$(CO)
OBJECTS = $(OBJECTS) gsm_destroy$(CO) gsm_option$(CO) lpc$(CO) rpe$(CO)
OBJECTS = $(OBJECTS) table$(CO)

gsm610$(LBO): $(LIBS) $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LIBS) $(LBENDL)

clean: $(MDUMMY)
	-@del gsm610$(LBO) $(OBJECTS) 2> nul

add.c:		gsm610_priv.h
code.c:		gsm610_priv.h
decode.c:	gsm610_priv.h
gsm_create.c:	config.h gsm.h gsm610_priv.h
gsm_decode.c:	gsm610_priv.h gsm.h
gsm_destroy.c:	gsm.h config.h
gsm_encode.c:	gsm610_priv.h gsm.h
gsm_option.c:	gsm610_priv.h gsm.h
long_term.c:	gsm610_priv.h
lpc.c:		gsm610_priv.h
preprocess.c:	gsm610_priv.h
rpe.c:		gsm610_priv.h
short_term.c:	gsm610_priv.h
table.c:	gsm610_priv.h
