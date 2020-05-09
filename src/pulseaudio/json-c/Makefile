#AR = ar
#CC = gcc
#CFLAGS ?= -Wall -Os -fPIC
#LDFLAGS = -L.
#SHLIB_CFLAGS = -shared

#INSTALL_EXEC = install -m 755 -o root -g root
#INSTALL_DATA = install -m 644 -o root -g root
#COPY_PRESERVELINKS = cp -d
#INSTALL_SOLINKS = $(COPY_PRESERVELINKS)

NAME = json
A_TARGETS = lib$(NAME)$(LBO)
#BIN_TARGETS = $(NAME)lint
#PC_TARGET = lib$(NAME).pc
#SO_LINKS = lib$(NAME).so lib$(NAME).so.$(MAJOR) lib$(NAME).so.$(MAJOR).$(MINOR)
#SO_FILE = lib$(NAME).so.$(MAJOR).$(MINOR).$(MICRO)
HEADERS = $(NAME).h

#PREFIX ?= /usr
#DESTDIR ?=
#INSTALLDIR ?= $(DESTDIR)$(PREFIX)

#TARGETS = $(A_TARGETS) $(SO_FILE) $(SO_LINKS) $(BIN_TARGETS) $(PC_TARGET)

TOPDIR   = ..\..
INCDIR   = $(TOPDIR)\utils;$(TOPDIR)\include
TARGET   = $(A_TARGETS)

include $(TOPDIR)\config\makerules

OBJECTS = arraylist$(CO) debug$(CO) json_object$(CO) json_tokener$(CO) json_util$(CO)  \
	linkhash$(CO) printbuf$(CO) 

lib$(NAME)$(LBO): $(OBJECTS)
	-@del $@ 2>nul
	$(LB) $@ $(OBJECTS) $(LBENDL)
#	$(AR) rc $@ $+

#lib$(NAME).so: lib$(NAME).so.$(MAJOR)
#	ln -sf $< $@

#lib$(NAME).so.$(MAJOR): lib$(NAME).so.$(MAJOR).$(MINOR)
#	ln -sf $< $@

#lib$(NAME).so.$(MAJOR).$(MINOR): lib$(NAME).so.$(MAJOR).$(MINOR).$(MICRO)
#	ln -sf $< $@

#lib$(NAME).so.$(MAJOR).$(MINOR).$(MICRO): $(NAME).o
#	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-soname -Wl,lib$(NAME).so.$(MAJOR).$(MINOR).$(MICRO) $(SHLIB_CFLAGS) -o $@ $^

#$(NAME)lint: $(NAME)lint.o $(NAME).o
#	$(CC) $(CFLAGS) -o $@ $+

#%.o: %.c %.h
#	$(CC) $(CFLAGS) -c -o $@ $<

#.PHONY: lib$(NAME).pc
#lib$(NAME).pc: lib$(NAME).pc.in
#	sed -e 's;@PREFIX@;$(PREFIX);' -e 's;@LIBJSON_VER_MAJOR@;$(MAJOR);' -e 's;@LIBJSON_VER_MINOR@;$(MINOR);' < $< > $@

#.PHONY: tests clean install install-bin install-lib
#tests: $(NAME)lint
#	(cd tests; ./runtest)

#install-lib: $(SO_TARGETS) $(A_TARGETS) $(PC_TARGET)
#	mkdir -p $(INSTALLDIR)/lib/pkgconfig
#	$(INSTALL_DATA) -t $(INSTALLDIR)/lib/pkgconfig $(PC_TARGET)
#	mkdir -p $(INSTALLDIR)/include
#	$(INSTALL_DATA) -t $(INSTALLDIR)/include $(HEADERS)
#	mkdir -p $(INSTALLDIR)/lib
#	$(INSTALL_EXEC) -t $(INSTALLDIR)/lib $(SO_FILE)
#	$(INSTALL_DATA) -t $(INSTALLDIR)/lib $(A_TARGETS)
#	$(INSTALL_SOLINKS) $(SO_LINKS) $(INSTALLDIR)/lib

#install-bin: $(BIN_TARGETS)
#	mkdir -p $(INSTALLDIR)/bin
#	$(INSTALL_EXEC) -t $(INSTALLDIR)/bin $(BIN_TARGETS)

#install: install-lib install-bin

clean: $(MDUMMY)
	-@del $(OBJECTS) $(TARGET) 2>nul

arraylist.o:	arraylist.c bits.h 
debug.o:	debug.c debug.h
json_object.o:	json_object.c debug.h linkhash.h json_object.h 
json_tokener.o:	json_tokener.c bits.h printbuf.h json_object.h 
json_util.o:	json_util.c bits.h printbuf.h json_tokener.h 
linkhash.o:	linkhash.c 
printbuf.o:	printbuf.c bits.h printbuf.h
test1.o:	test1.c 
test2.o:	test2.c 
test3.o:	test3.c 
