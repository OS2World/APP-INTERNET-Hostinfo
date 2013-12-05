# Makefile: how to build 'hostinfo' and make source tarball
# created 1998-Apr-19 jmk
# autodate: 2000-Jul-20 06:08
#
# by Jim Knoble <jmknoble@jmknoble.cx>
# Copyright (C) 1998,1999,2000 Jim Knoble
#
# THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# express or implied, including but not limited to the warranties of
# merchantability, fitness for a particular purpose and
# noninfringement. In no event shall the author(s) be liable for any
# claim, damages or other liability, whether in an action of
# contract, tort or otherwise, arising from, out of or in connection
# with the software or the use or other dealings in the software.
#
# Permission to use, copy, modify, distribute, and sell this software
# and its documentation for any purpose is hereby granted without
# fee, provided that the above copyright notice appear in all copies
# and that both that copyright notice and this permission notice
# appear in supporting documentation.

SHELL		= /bin/sh

NAME		= hostinfo
VERSION		= 2.2
DATE		= 2000-Jul-20

DESTDIR		= 
prefix		= /progetti/hostinfo-2.2/os2
bindir		= $(prefix)/bin

CC		= gcc
OPTFLAGS	= -O2
WARNFLAGS	= -Wall -W -ansi -pedantic 
MISCFLAGS	= -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DDATE=\"$(DATE)\"
CFLAGS		= $(OPTFLAGS) $(WARNFLAGS) $(MISCFLAGS)
LDFLAGS		=  -Zexe

INSTALL		= install
INSTALL_BIN	= $(INSTALL) -m 0755 -s
INSTALL_MAN	= $(INSTALL) -m 0444
INSTALL_SCRIPT	= $(INSTALL) -m 0755
INSTALL_DATA	= $(INSTALL) -m 0644

CHMOD		= chmod
CP		= cp
GZIP		= gzip -9
MKDIRHIER	= mkdir -p
MV		= mv -f
RM		= rm -f
TAR		= tar

PROG		= $(NAME)

HDRS		= 
SRCS		= $(PROG).c
OBJS		= $(SRCS:.c=.o)

MAN_SRC		= 
MAN_OBJ		= 

SPEC_OBJ	= $(NAME).spec
SPEC_SRC	= $(SPEC_OBJ).in

CHANGELOG	= ChangeLog
MAKEFILES	= Makefile
DOCS		= 
EXTRAS		= $(CHANGELOG)

PACKAGE_SPEC	= $(NAME)-$(VERSION)
DIST_DIR	= $(PACKAGE_SPEC)
DIST		= $(DIST_DIR).tar.gz
DIST_FILES	= \
 $(MAKEFILES) \
 $(DOCS) \
 $(EXTRAS) \
 $(SPEC_SRC) \
 $(SPEC_OBJ) \
 $(SRCS) \
 $(HDRS) \
 $(MAN_SRC)

.PHONY: all install includes sed spec changelog dist tar
.PHONY: clean realclean spotless distclean

.SUFFIXES: 
.SUFFIXES: .c .o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

install: $(PROG) $(DESTDIR)$(bindir)
	$(INSTALL_BIN) $(PROG) $(DESTDIR)$(bindir)

$(DESTDIR)$(bindir):
	$(MKDIRHIER) $@

includes: spec
sed: spec
spec: $(SPEC_OBJ)

$(SPEC_OBJ): $(SPEC_SRC) $(MAKEFILES)
	cat $(SPEC_SRC) |sed \
	  -e 's,@NAME@,$(NAME),g' \
	  -e 's,@VERSION@,$(VERSION),g' \
	  -e 's,@DATE@,$(DATE),g' \
	>$@

changelog: $(CHANGELOG)
$(CHANGELOG): $(SRCS) $(HDRS) $(MAKEFILES) $(SPEC_SRC)
	cvs2cl

clean:
	$(RM) $(PROG) $(OBJS)

realclean: clean
	$(RM) *~ *.LOG *.log *.bak

distclean: realclean
	$(RM) $(DIST)

spotless: distclean
	$(RM) $(SPEC_OBJ) $(CHANGELOG)

tar: dist
dist: $(DIST)

$(DIST): $(DIST_FILES)
	$(RM) -r ./$(DIST_DIR)
	$(RM) $@.tmp
	$(MKDIRHIER) ./$(DIST_DIR)
	$(CP) -p $(DIST_FILES) ./$(DIST_DIR)/
	$(CHMOD) -R u+rwX,go+rX,go-w ./$(DIST_DIR)
	$(TAR) -cvf - ./$(DIST_DIR) |$(GZIP) -c >$@.tmp
	$(MV) $@.tmp $@
	$(RM) -r ./$(DIST_DIR)

