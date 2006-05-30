#
#  skinutil.exe makefile for NMAKE and VAC++
#


!include ..\config\makerules

INCLUDE = $(INCLUDE);..\utils;..\gbm123;..\pm123
LIB     = $(LIB);..\utils;..\gbm123

all: skinutil.exe

DEF     = skinutil.def
OBJECTS = skinutil.obj 
LIBS    = gbm123.lib gbmrect.lib utilfct.lib

skinutil.exe: $(OBJECTS) $(DEF) $(RESOURCE)
   $(CC) $(CFLAGS) /Fm$* /Fe$@ $(OBJECTS) $(DEF) $(LIBS)

clean:
  -@del skinutil.exe skinutil.map $(OBJECTS) $(RESOURCE) 2> nul
