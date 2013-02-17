#
#  libogg library makefile
#

DLL      = yes
TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\include;..\include

!include $(TOPDIR)\config\makerules

all: libogg$(LBO) $(MDUMMY)

OBJECTS = bitwise$(CO) framing$(CO)

libogg$(LBO): $(OBJECTS)
	if exist $@ del $@
	$(LB) $@ $(OBJECTS) $(LBENDL)

clean: $(MDUMMY)
	-@del libogg$(LBO) 2> nul
	-@del $(OBJECTS)   2> nul
