#
#  libogg library makefile
#

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..\include

include $(TOPDIR)\config\makerules

all: libogg$(LBO) $(MDUMMY)

OBJECTS = bitwise$(CO) framing$(CO)

libogg$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libogg$(LBO) 2> nul
	-@del $(OBJECTS)   2> nul

bitwise.o:	bitwise.c 
framing.o:	framing.c 
