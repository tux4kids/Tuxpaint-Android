# Tux Paint - A simple drawing program for children.

# Copyright (c) 2002-2014 by Bill Kendrick and others
# bill@newbreedsoftware.com
# http://www.tuxpaint.org/

# June 14, 2002 - March 30, 2014


# The version number, for release:

VER_VERSION:=0.9.22
VER_DATE:=$(shell date +"%Y-%m-%d")
MAGIC_API_VERSION:=0x00000003

# Need to know the OS

SYSNAME:=$(shell uname -s)
ifeq ($(findstring MINGW32, $(SYSNAME)),MINGW32)
OS:=windows
GPERF:=/usr/bin/gperf
else
ifeq ($(SYSNAME),Darwin)
OS:=osx
GPERF:=/usr/bin/gperf
else
ifeq ($(SYSNAME),BeOS)
OS:=beos
GPERF:=$(shell finddir B_USER_BIN_DIRECTORY)/gperf
else
ifeq ($(SYSNAME),Haiku)
OS:=beos
GPERF:=$(shell finddir B_SYSTEM_BIN_DIRECTORY)/gperf
STDC_LIB:=-lstdc++
ifeq ($(shell gcc --version | cut -c 1-6),2.95.3)
STDC_LIB:=-lstdc++.r4
endif
else
OS:=linux
GPERF:=/usr/bin/gperf
endif
endif
endif
endif

# change to sdl-console to build a console version on Windows
SDL_PCNAME:=sdl2

WINDRES:=windres
PKG_CONFIG:=pkg-config

# test if a library can be linked
linktest = $(shell if $(CC) $(CPPFLAGS) $(CFLAGS) -o dummy.o dummy.c $(LDFLAGS) $(1) $(2) > /dev/null 2>&1; \
	then \
		echo "$(1)"; \
	fi ;)

# test compiler options
comptest = $(shell if $(CC) $(CPPFLAGS) $(CFLAGS) $(1) $(2) -o dummy.o dummy.c $(LDFLAGS) > /dev/null 2>&1; \
	then \
		echo "$(1)"; \
	fi ;)

beos_RSRC_CMD:=rc haiku/tuxpaint.rdef && xres -o tuxpaint haiku/tuxpaint.rsrc
RSRC_CMD:=$($(OS)_RSRC_CMD)

beos_MIMESET_CMD:=mimeset -f tuxpaint
MIMESET_CMD:=$($(OS)_MIMESET_CMD)

windows_SO_TYPE:=dll
osx_SO_TYPE:=bundle
beos_SO_TYPE:=so
linux_SO_TYPE:=so
SO_TYPE:=$($(OS)_SO_TYPE)

windows_LIBMINGW:=-lmingw32
LIBMINGW:=$($(OS)_LIBMINGW)

windows_EXE_EXT:=.exe
EXE_EXT:=$($(OS)_EXE_EXT)

windows_ARCH_LIBS:=obj/win32_print.o obj/resource.o
osx_ARCH_LIBS:=obj/postscript_print.o
beos_ARCH_LIBS:=obj/BeOS_print.o
linux_ARCH_LIBS:=obj/postscript_print.o
ARCH_LIBS:=$($(OS)_ARCH_LIBS)

PAPER_LIB:=$(call linktest,-lpaper,)
PNG:=$(call linktest,-lpng,)
PNG:=$(if $(PNG),$(PNG),$(call linktest,-lpng12,))

FRIBIDI_LIB:=$(shell $(PKG_CONFIG) --libs fribidi)
FRIBIDI_CFLAGS:=$(shell $(PKG_CONFIG) --cflags fribidi)

windows_ARCH_LINKS:=-lintl $(PNG) -lzdll -lwinspool -lshlwapi $(FRIBIDI_LIB) -liconv
osx_ARCH_LINKS:=$(PAPER_LIB) $(FRIBIDI_LIB)
beos_ARCH_LINKS:=-lintl $(PNG) -lz -lbe -lnetwork -liconv $(FRIBIDI_LIB) $(PAPER_LIB) $(STDC_LIB)
linux_ARCH_LINKS:=$(PAPER_LIB) $(FRIBIDI_LIB)
ARCH_LINKS:=$($(OS)_ARCH_LINKS)

windows_ARCH_HEADERS:=src/win32_print.h
osx_ARCH_HEADERS:=
beos_ARCH_HEADERS:=src/BeOS_print.h
linux_ARCH_HEADERS:=
ARCH_HEADERS:=$($(OS)_ARCH_HEADERS)

# Where things will go when ultimately installed:
windows_PREFIX:=/usr/local
osx_PREFIX:=/usr/local

beos_PREFIX=$(shell finddir B_APPS_DIRECTORY)/TuxPaint
linux_PREFIX:=/usr/local
PREFIX:=$($(OS)_PREFIX)

# Root directory to place files when creating packages.
# PKG_ROOT is the old name for this, and should be undefined.
# "TuxPaint-1" is the OLPC XO name. Installing to ./ is bad!
ifeq ($(PREFIX),./)
DESTDIR:=TuxPaint-1
else
DESTDIR:=$(PKG_ROOT)
endif

# Program:
BIN_PREFIX:=$(DESTDIR)$(PREFIX)/bin

# Data:
DATA_PREFIX:=$(DESTDIR)$(PREFIX)/share/tuxpaint

# Locale files
LOCALE_PREFIX=$(DESTDIR)$(PREFIX)/share/locale

# IM files
IM_PREFIX=$(DESTDIR)$(PREFIX)/share/tuxpaint/im

# Libraries
LIBDIR=$(PREFIX)

# Magic Tool plug-ins
INCLUDE_PREFIX:=$(DESTDIR)$(PREFIX)/include
MAGIC_PREFIX:=$(DESTDIR)$(LIBDIR)/lib/tuxpaint/plugins


# Docs and man page:
DOC_PREFIX:=$(DESTDIR)$(PREFIX)/share/doc/tuxpaint
DEVDOC_PREFIX:=$(DESTDIR)$(PREFIX)/share/doc/tuxpaint-dev
MAN_PREFIX:=$(DESTDIR)$(PREFIX)/share/man
DEVMAN_PREFIX:=$(DESTDIR)$(PREFIX)/share/man

# BASH tab-completion file:
COMPLETIONDIR:=$(DESTDIR)/etc/bash_completion.d

# 'System-wide' Config file:
ifeq ($(PREFIX),/usr)
  CONFDIR:=$(DESTDIR)/etc/tuxpaint
else
  CONFDIR:=$(DESTDIR)$(PREFIX)/etc/tuxpaint
endif

ifeq ($(SYSNAME),Haiku)
  CONFDIR:=$(shell finddir B_USER_SETTINGS_DIRECTORY)/TuxPaint
endif

# Icons and launchers:
ICON_PREFIX:=$(DESTDIR)$(PREFIX)/share/pixmaps
X11_ICON_PREFIX:=$(DESTDIR)$(PREFIX)/X11R6/include/X11/pixmaps
GNOME_PREFIX:=$(shell gnome-config --prefix 2> /dev/null)
KDE_PREFIX:=$(shell kde-config --install apps --expandvars 2> /dev/null)
KDE_ICON_PREFIX:=$(shell kde-config --install icon --expandvars 2> /dev/null)

# Maemo flag
MAEMOFLAG:=


# Where to find cursor shape XBMs

MOUSEDIR:=mouse
CURSOR_SHAPES:=LARGE
# MOUSEDIR:=mouse/16x16
# CURSOR_SHAPES:=SMALL

# Libraries, paths, and flags:
SDL_LIBS:=$(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf -lz $(PNG)

# Sound support
SDL_MIXER_LIB:=$(call linktest,-lSDL2_mixer,$(SDL_LIBS))
NOSOUNDFLAG:=$(if $(SDL_MIXER_LIB),,-DNOSOUND$(warning -lSDL2_Mixer failed, no sound for you!))

# SDL Pango is needed to render complex scripts like Thai and Arabic
SDL2_PANGO_LIB:=$(call linktest,-lSDL2_Pango,$(SDL_LIBS))
NOPANGOFLAG:=$(if $(SDL2_PANGO_LIB),,-DNO_SDLPANGO$(warning -lSDL2_Pango failed, no scripts for you!))

SDL_LIBS+=$(SDL_MIXER_LIB) $(SDL2_PANGO_LIB)

SDL_CFLAGS:=$(shell $(PKG_CONFIG) $(SDL_PCNAME) --cflags)


# New one: -lrsvg-2 -lcairo
# Old one: -lcairo -lsvg -lsvg-cairo
SVG_LIB:=$(shell $(PKG_CONFIG) --libs librsvg-2.0 cairo || $(PKG_CONFIG) --libs libsvg-cairo)

# lots of -I things, so really should be SVG_CPPFLAGS
SVG_CFLAGS:=$(shell $(PKG_CONFIG) --cflags librsvg-2.0 cairo || $(PKG_CONFIG) --cflags libsvg-cairo)

# SVG support via Cairo
NOSVGFLAG:=$(if $(SVG_LIB),,-DNOSVG$(warning No SVG for you!))

# SVG support uses libcairo1
OLDSVGFLAG:=$(if $(filter -lsvg-cairo,$(SVG_LIB)),-DOLD_SVG,)


ifeq ($(hack),1)
hack:
	@echo 'SDL2_PANGO_LIB is' $(SDL2_PANGO_LIB)
	@echo 'SDL_MIXER_LIB is' $(SDL_MIXER_LIB)
	@echo 'SVG_LIB       is' $(SVG_LIB)
	@echo 'SDL_LIBS      is' $(SDL_LIBS)
	@echo 'SDL_CFLAGS    is' $(SDL_CFLAGS)
	@echo 'SVG_CFLAGS    is' $(SVG_CFLAGS)
	@echo 'LDFLAGS       is' $(LDFLAGS)
	@echo 'CFLAGS        is' $(CFLAGS)
	@echo 'CPPFLAGS      is' $(CPPFLAGS)
endif

# The entire set of CFLAGS:

#-ffast-math
OPTFLAGS:=-O0 -g
CFLAGS:=$(CPPFLAGS) $(OPTFLAGS) -W -Wall -fno-common -ffloat-store \
	$(if $(filter windows,$(OS)),,$(call comptest,-fvisibility=hidden,)) \
	-Wcast-align -Wredundant-decls \
	-Wbad-function-cast -Wwrite-strings \
	-Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	$(shell src/test-option.sh -Wstrict-aliasing=2)

DEFS:=-DVER_DATE=\"$(VER_DATE)\" -DVER_VERSION=\"$(VER_VERSION)\" \
	-DDATA_PREFIX=\"$(patsubst $(DESTDIR)%,%,$(DATA_PREFIX))/\" \
	-DDOC_PREFIX=\"$(patsubst $(DESTDIR)%,%,$(DOC_PREFIX))/\" \
	-DLOCALEDIR=\"$(patsubst $(DESTDIR)%,%,$(LOCALE_PREFIX))/\" \
	-DIMDIR=\"$(patsubst $(DESTDIR)%,%,$(IM_PREFIX))/\" \
	-DCONFDIR=\"$(patsubst $(DESTDIR)%,%,$(CONFDIR))/\" \
	-DMAGIC_PREFIX=\"$(patsubst $(DESTDIR)%,%,$(MAGIC_PREFIX))/\" \
	$(NOSOUNDFLAG) $(NOSVGFLAG) $(OLDSVGFLAG) $(NOPANGOFLAG) \
	$(MAEMOFLAG)

DEBUG_FLAGS:=
#DEBUG_FLAGS:=-g

MOUSE_CFLAGS:=-Isrc/$(MOUSEDIR) -D$(CURSOR_SHAPES)_CURSOR_SHAPES

.SUFFIXES:

#############################################################################
#############################################################################
#############################################################################
#
# "make" with no arguments builds the program and man page from sources:
#
.PHONY: all
all:	tuxpaint translations magic-plugins tp-magic-config
# thumb-starters
	@echo
	@echo "--------------------------------------------------------------"
	@echo
	@echo "Done compiling."
	@echo
	@echo "Now run 'make install' with any options you ran 'make' with."
	@echo "to install Tux Paint."
	@echo
	@echo "You may need superuser ('root') privileges, depending on"
	@echo "where you're installing."
	@echo "(Depending on your system, you either need to 'su' first,"
	@echo "or run 'sudo make install'.)"
	@echo

.PHONY: releaseclean
releaseclean:
	@echo
	@echo "Cleaning release directory"
	@echo
	@rm -rf "build/tuxpaint-$(VER_VERSION)" "build/tuxpaint-$(VER_VERSION).tar.gz"
	@-if [ -d build ] ; then rmdir build ; fi

.PHONY: releasedir
releasedir: build/tuxpaint-$(VER_VERSION)


build/tuxpaint-$(VER_VERSION):
	@echo
	@echo "Creating release directory"
	@echo
	@mkdir -p build/tuxpaint-$(VER_VERSION)
	@find . -follow \
	     \( -wholename '*/CVS' -o -name .thumbs -o -name .cvsignore -o -name 'dummy.o' -o -name 'build' -o -name '.#*' \) \
	     -prune -o -type f -exec cp --parents -vdp \{\} build/tuxpaint-$(VER_VERSION)/ \;

.PHONY: release
release: releasedir
	@echo
	@echo "Creating release tarball"
	@echo
	@cd build ; \
	    tar -czvf tuxpaint-$(VER_VERSION).tar.gz tuxpaint-$(VER_VERSION)

# "make olpc" builds the program for an OLPC XO:
MAGIC_GOOD:=blur blocks_chalk_drip bricks calligraphy fade_darken\
            fill flower foam grass mirror_flip shift smudge snow tint
.PHONY: olpc
olpc:
	@echo
	@echo "Building for an OLPC XO"
	@echo
	make PREFIX:=. MAGIC_C:=$(patsubst %,magic/src/%.c,$(MAGIC_GOOD)) OPTFLAGS:='-O2 -fno-tree-pre -march=athlon -mtune=generic -mpreferred-stack-boundary=2 -mmmx -m3dnow -fomit-frame-pointer -falign-functions=0 -falign-jumps=0 -DOLPC_XO -DSUGAR'

# "make nokia770" builds the program for the Nokia 770.
.PHONY: nokia770
nokia770:
	make \
		DATA_PREFIX:=/usr/share/tuxpaint \
		MAEMOFLAG:=-DNOKIA_770 \
		LOCALE_PREFIX:=$(PREFIX)/share/locale \
		CONFDIR:=/etc/tuxpaint

##### i18n stuff

POFILES:=$(wildcard src/po/*.po)
MOFILES:=$(patsubst src/po/%.po,trans/%.mo,$(POFILES))
INSTALLED_MOFILES:=$(patsubst trans/%.mo,$(LOCALE_PREFIX)/%/LC_MESSAGES/tuxpaint.mo,$(MOFILES))
INSTALLED_MODIRS:=$(patsubst trans/%.mo,$(LOCALE_PREFIX)/%/LC_MESSAGES,$(MOFILES))

$(INSTALLED_MODIRS): $(LOCALE_PREFIX)/%/LC_MESSAGES: trans/%.mo
	install -d -m 755 $@
$(INSTALLED_MOFILES): $(LOCALE_PREFIX)/%/LC_MESSAGES/tuxpaint.mo: trans/%.mo
	install -m 644 $< $@

.PHONY: uninstall-i18n
uninstall-i18n:
	-rm $(LOCALE_PREFIX)/*/LC_MESSAGES/tuxpaint.mo
	-rm $(IM_PREFIX)/ja.im
	-rm $(IM_PREFIX)/ko.im
	-rm $(IM_PREFIX)/th.im
	-rm $(IM_PREFIX)/zh_tw.im

##### i18n stuff for Tux Paint Config bundling
TPCONF_PATH:=../tuxpaint-config
TPCPOFILES:=$(wildcard $(TPCONF_PATH)/src/po/*.po)
TPCMOFILES:=$(patsubst $(TPCONF_PATH)/src/po/%.po,$(TPCONF_PATH)/trans/%.mo,$(TPCPOFILES))
TPCINSTALLED_MOFILES:=$(patsubst $(TPCONF_PATH)/trans/%.mo,$(LOCALE_PREFIX)/%/LC_MESSAGES/tuxpaint-config.mo,$(TPCMOFILES))

$(TPCINSTALLED_MOFILES): $(LOCALE_PREFIX)/%/LC_MESSAGES/tuxpaint-config.mo: $(TPCONF_PATH)/trans/%.mo
	@echo
	@echo "...Installing Tux Paint Config i18n..."
	install -D -m 644 $< $@

install-tpconf-i18n: $(TPCINSTALLED_MOFILES)

# Install the translated text:
# We can install *.mo files if they were already generated, or if it can be
# generated from the *.po files.  The *.mo files can be generated from the
# *.po files if we have the converter program, msgfmt, installed in the
# system.  So we test for both and install them if either case is found
# to be true.  If neither case is found to be true, we'll just install
# Tux Paint without the translation files.
.PHONY: install-gettext
ifeq "$(wildcard trans/*.mo)$(shell msgfmt -h)" ""
install-gettext:
	@echo
	@echo "--------------------------------------------------------------"
	@echo "Cannot install translation files because no translation files"
	@echo "were found (trans/*.mo) and the 'msgfmt' program is not installed."
	@echo "You will not be able to run Tux Paint in non-U.S. English modes."
	@echo "--------------------------------------------------------------"
else
install-gettextdirs: $(INSTALLED_MODIRS)
install-gettext: install-gettextdirs $(INSTALLED_MOFILES)
endif


# Install the Input Method files:
.PHONY: install-im
ifneq ($(IM_PREFIX),)
install-im:
	@echo
	@echo "...Installing Input Method files..."
	@#
	@install -d $(IM_PREFIX)
	@#
	@echo "   ja ...Japanese..."
	@cp im/ja.im $(IM_PREFIX)/ja.im
	@chmod 644 $(IM_PREFIX)/ja.im
	@#
	@echo "   ko ...Korean..."
	@cp im/ko.im $(IM_PREFIX)/ko.im
	@chmod 644 $(IM_PREFIX)/ko.im
	@#
	@echo "   th ...Thai..."
	@cp im/th.im $(IM_PREFIX)/th.im
	@chmod 644 $(IM_PREFIX)/th.im
	@#
	@echo "   zh_tw ...Traditional Chinese..."
	@cp im/zh_tw.im $(IM_PREFIX)/zh_tw.im
	@chmod 644 $(IM_PREFIX)/zh_tw.im
else
install-im:
	@echo
	@echo "...Not Installing Input Method files (no IM_PREFIX defined)..."
endif


# Build the translation files for gettext

$(MOFILES): trans/%.mo: src/po/%.po  
	msgfmt -o $@ $<

.PHONY: translations
ifeq "$(shell msgfmt -h)" ""
translations: trans
	@echo "--------------------------------------------------------------"
	@echo "Cannot find program 'msgfmt'!"
	@echo "No translation files will be prepared."
	@echo "Install gettext to run Tux Paint in non-U.S. English modes."
	@echo "--------------------------------------------------------------"
else
translations: trans $(MOFILES)
endif

trans:
	@echo
	@echo "...Preparing translation files..."
	@mkdir trans

######

windows_ARCH_INSTALL:=
osx_ARCH_INSTALL:=
beos_ARCH_INSTALL:=install-haiku
linux_ARCH_INSTALL:=install-gnome install-kde install-kde-icons
ARCH_INSTALL:=$($(OS)_ARCH_INSTALL)

# "make install" installs all of the various parts
# (depending on the *PREFIX variables at the top, you probably need
# to do this as superuser ("root"))
.PHONY: install
install:	install-bin install-data install-man install-doc \
		install-magic-plugins \
		install-magic-plugin-dev \
		install-icon install-gettext install-im install-importscript \
		install-default-config install-example-stamps \
		install-example-starters install-example-templates \
		install-bash-completion \
		install-osk \
		$(ARCH_INSTALL)
#install-thumb-starters
	@echo
	@echo "--------------------------------------------------------------"
	@echo
	@echo "All done! Now (preferably NOT as 'root' superuser),"
	@echo "you can type the command 'tuxpaint' to run the program!!!"
	@echo
	@echo "For more information, see the 'tuxpaint' man page,"
	@echo "run 'tuxpaint --usage' or see $(DOC_PREFIX)/README.txt"
	@echo
	@echo "Visit Tux Paint's home page for more information, updates"
	@echo "and to learn how you can help out!"
	@echo
	@echo "  http://www.tuxpaint.org/"
	@echo
	@echo "Enjoy!"
	@echo

.PHONY: install-magic-plugins
install-magic-plugins:
	@echo
	@echo "...Installing Magic Tool plug-ins..."
	@install -d $(MAGIC_PREFIX)
	@cp magic/*.$(SO_TYPE) $(MAGIC_PREFIX)
	@chmod a+r,g-w,o-w $(MAGIC_PREFIX)/*.$(SO_TYPE)
	@install -d $(DATA_PREFIX)/images/magic
	@cp magic/icons/*.png $(DATA_PREFIX)/images/magic
	@chmod a+r,g-w,o-w $(DATA_PREFIX)/images/magic/*.png
	@install -d $(DATA_PREFIX)/sounds/magic
	@cp magic/sounds/*.wav magic/sounds/*.ogg $(DATA_PREFIX)/sounds/magic
	@chmod a+r,g-w,o-w $(DATA_PREFIX)/sounds/magic/*.wav \
			$(DATA_PREFIX)/sounds/magic/*.ogg

.PHONY: install-magic-plugins
install-magic-plugin-dev:	src/tp_magic_api.h
	@echo
	@echo "...Installing Magic Tool plug-in development files and docs..."
	@cp tp-magic-config $(BIN_PREFIX)
	@chmod a+rx,g-w,o-w $(BIN_PREFIX)/tp-magic-config
	@install -d $(INCLUDE_PREFIX)/tuxpaint
	@cp src/tp_magic_api.h $(INCLUDE_PREFIX)/tuxpaint
	@chmod a+r,g-w,o-w $(INCLUDE_PREFIX)/tuxpaint/tp_magic_api.h
	@install -d $(DEVDOC_PREFIX)
	@cp -R magic/docs/* $(DEVDOC_PREFIX)
	@chmod a=rX,g=rX,u=rwX $(DEVDOC_PREFIX)

# Installs the various parts for the MinGW/MSYS development/testing environment.

# "make bdist-win32" recompiles Tux Paint to work with executable-relative
# data, docs and locale directories. Also copies all files, including DLLs,
# into a 'bdist' output directory ready for processing by an installer script.
.PHONY: bdist-win32
bdist-win32:
	@-rm -f tuxpaint.exe
	@-rm -f obj/*.o
	make \
		PREFIX:=./win32/bdist \
		DATA_PREFIX:=data \
		DOC_PREFIX:=docs \
		LOCALE_PREFIX:=locale \
		IM_PREFIX:=im \
		CONFDIR:=. \
		COMPLETIONDIR:=. \
		INCLUDE_PREFIX:=plugins/include \
		MAGIC_PREFIX:=plugins
	strip -s tuxpaint.exe
	make install \
		PREFIX:=./win32/bdist \
		BIN_PREFIX:=./win32/bdist \
		DATA_PREFIX:=./win32/bdist/data \
		DOC_PREFIX:=./win32/bdist/docs \
		LOCALE_PREFIX:=./win32/bdist/locale \
		IM_PREFIX:=./win32/bdist/im \
		CONFDIR:=./win32/bdist \
		COMPLETIONDIR:=./win32/bdist \
		INCLUDE_PREFIX:=./win32/bdist/plugins/include \
		MAGIC_PREFIX:=./win32/bdist/plugins \
		windows_ARCH_INSTALL:=install-dlls install-tpconf-i18n

# "make bdist-clean" deletes the 'bdist' directory
.PHONY: bdist-clean
bdist-clean:
	@echo
	@echo "Cleaning up the 'bdist' directory! ($(PWD))"
	@-rm -rf ./win32/bdist
	@echo

# "make clean" deletes the program, the compiled objects and the
# built man page (returns to factory archive, pretty much...)
.PHONY: clean
clean:
	@echo
	@echo "Cleaning up the build directory! ($(PWD))"
	@-rm -f magic/*.$(SO_TYPE)
	@-rm -f tuxpaint
	@-rm -f obj/*.o
	@-rm -f obj/parse.c obj/parse_step1.c
	@-rm -f dummy.o
	@#if [ -d obj ]; then rmdir obj; fi
	@-rm -f trans/*.mo
	@-rm -f src/tp_magic_api.h
	@-rm -f tp-magic-config
	@if [ -d trans ]; then rmdir trans; fi
	@-rm -f starters/.thumbs/*.png
	@if [ -d starters/.thumbs ]; then rmdir starters/.thumbs; fi
	@-rm -f templates/.thumbs/*.png
	@if [ -d templates/.thumbs ]; then rmdir templates/.thumbs; fi
	@echo

# "make uninstall" should remove the various parts from their
# installation locations.  BE SURE the *PREFIX variables at the top
# are the same as they were when you installed, of course!!!
.PHONY: uninstall
uninstall:	uninstall-i18n
	-if [ "x$(GNOME_PREFIX)" != "x" ]; then \
	  rm $(GNOME_PREFIX)/share/applications/tuxpaint.desktop; \
	  rm $(GNOME_PREFIX)/share/pixmaps/tuxpaint.png; \
	else \
	  rm /usr/share/applications/tuxpaint.desktop; \
	  rm /usr/share/pixmaps/tuxpaint.png; \
	fi
	-if [ "x$(KDE_PREFIX)" != "x" ]; then \
	  rm $(KDE_PREFIX)/Graphics/tuxpaint.desktop; \
	fi
	-rm $(ICON_PREFIX)/tuxpaint.png
	-rm $(X11_ICON_PREFIX)/tuxpaint.xpm
	-if [ "x$(KDE_ICON_PREFIX)" != "x" ]; then \
	  rm $(KDE_ICON_PREFIX)/hicolor/scalable/apps/tuxpaint.svg; \
	  rm $(KDE_ICON_PREFIX)/hicolor/192x192/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/128x128/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/96x96/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/64x64/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/48x48/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/32x32/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/22x22/apps/tuxpaint.png; \
	  rm $(KDE_ICON_PREFIX)/hicolor/16x16/apps/tuxpaint.png; \
	fi
	-rm $(BIN_PREFIX)/tuxpaint
	-rm $(BIN_PREFIX)/tuxpaint-import
	-rm -r $(DATA_PREFIX)
	-rm -r $(DOC_PREFIX)
	-rm $(MAN_PREFIX)/man1/tuxpaint.1.gz
	-rm $(MAN_PREFIX)/pl/man1/tuxpaint.1.gz
	-rm $(MAN_PREFIX)/man1/tuxpaint-import.1.gz
	-rm $(MAN_PREFIX)/man1/tp-magic-config.1.gz
	-rm -f -r $(CONFDIR)
	-rm $(COMPLETIONDIR)/tuxpaint-completion.bash
	-rm -r $(MAGIC_PREFIX)
	-rm -r $(INCLUDE_PREFIX)/tuxpaint
	-rm $(BIN_PREFIX)/tp-magic-config
	-rm -r $(DEVDOC_PREFIX)


# Install default config file:
.PHONY: install-default-config
install-default-config:
	@echo
	@echo "...Installing default config file..."
	@install -d $(CONFDIR)
	@cp src/tuxpaint.conf $(CONFDIR)
	@chmod 644 $(CONFDIR)/tuxpaint.conf

# Install BASH completion file:
.PHONY: install-bash-completion
install-bash-completion:
	@echo
	@echo "...Installing BASH completion file..."
	@install -d $(COMPLETIONDIR)
	@cp src/tuxpaint-completion.bash $(COMPLETIONDIR)
	@chmod 644 $(COMPLETIONDIR)/tuxpaint-completion.bash


# Install example stamps
.PHONY: install-example-stamps
install-example-stamps:
	@echo
	@echo "...Installing example stamps..."
	@install -d $(DATA_PREFIX)/stamps
	@cp -R stamps/* $(DATA_PREFIX)/stamps
	@chmod -R a+rX,g-w,o-w $(DATA_PREFIX)/stamps


STARTERS:=$(wildcard starters/*.*)
INSTALLED_STARTERS:=$(patsubst %,$(DATA_PREFIX)/%,$(STARTERS))

$(INSTALLED_STARTERS): $(DATA_PREFIX)/%: %
	install -m 644 $< $@

install-example-starters-dirs:
	install -d -m 755 $(DATA_PREFIX)/starters

.PHONY: echo-install-example-starters
echo-install-example-starters:
	@echo
	@echo "...Installing example starters..."

# Install example starters
.PHONY: install-example-starters
install-example-starters: echo-install-example-starters install-example-starters-dirs $(INSTALLED_STARTERS)

THUMB_STARTERS:=$(sort $(patsubst starters%, starters/.thumbs%-t.png, $(basename $(subst -back.,.,$(STARTERS)))))
INSTALLED_THUMB_STARTERS:=$(patsubst %,$(DATA_PREFIX)/%,$(THUMB_STARTERS))

STARTER_NAME=$(or $(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=.svg))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=.png))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=.jpeg))))

STARTER_BACK_NAME=$(or $(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.svg))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.png))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.jpeg))))

$(THUMB_STARTERS):
	@echo -n "."
	@mkdir -p starters/.thumbs
	@if [ "x" != "x"$(STARTER_BACK_NAME) ] ; \
	then \
		composite $(STARTER_NAME) $(STARTER_BACK_NAME) obj/tmp.png ; \
		convert -scale !132x80 -background white -alpha Background -alpha Off obj/tmp.png $@ ; \
		rm obj/tmp.png ; \
	else \
		convert -scale !132x80 -background white -alpha Background -alpha Off $(STARTER_NAME) $@ ; \
	fi

$(INSTALLED_THUMB_STARTERS): $(DATA_PREFIX)/%: %
	@install -D -m 644 $< $@

.PHONY: echo-thumb-starters
echo-thumb-starters:
	@echo
	@echo "...Generating thumbnails for starters..."

# Create thumbnails for starters
.PHONY: thumb-starters
thumb-starters: echo-thumb-starters $(THUMB_STARTERS)

.PHONY: echo-install-thumb-starters
echo-install-thumb-starters:
	@echo
	@echo "...Installing thumbnails for starters..."

# Install thumb starters
.PHONY: install-thumb-starters
install-thumb-starters: echo-install-thumb-starters $(INSTALLED_THUMB_STARTERS)


TEMPLATES:=$(wildcard templates/*.*)
INSTALLED_TEMPLATES:=$(patsubst %,$(DATA_PREFIX)/%,$(TEMPLATES))

$(INSTALLED_TEMPLATES): $(DATA_PREFIX)/%: %
	install -m 644 $< $@

install-example-template-dirs:
	install -d -m 755 $(DATA_PREFIX)/templates

.PHONY: echo-install-example-templates
echo-install-example-templates:
	@echo
	@echo "...Installing example templates..."

# Install example templates
.PHONY: install-example-templates
install-example-templates: echo-install-example-templates install-example-template-dirs $(INSTALLED_TEMPLATES)


# Install a launcher icon in the Gnome menu
.PHONY: install-gnome
install-gnome:
	@echo
	@echo "...Installing launcher icon into GNOME..."
	@if [ "x$(GNOME_PREFIX)" != "x" ]; then \
	 install -d $(DESTDIR)$(GNOME_PREFIX)/share/pixmaps; \
	 cp data/images/icon.png $(DESTDIR)/$(GNOME_PREFIX)/share/pixmaps/tuxpaint.png; \
	 chmod 644 $(DESTDIR)$(GNOME_PREFIX)/share/pixmaps/tuxpaint.png; \
	 install -d $(DESTDIR)$(GNOME_PREFIX)/share/applications; \
	 cp src/tuxpaint.desktop $(DESTDIR)$(GNOME_PREFIX)/share/applications/; \
	 chmod 644 $(DESTDIR)$(GNOME_PREFIX)/share/applications/tuxpaint.desktop; \
	fi


# Install a launcher icon for the Nokia 770.
.PHONY: install-nokia770
install-nokia770:
	@echo
	@echo "...Installing launcher icon into the Nokia 770..."
	@if [ "x$(NOKIA770_PREFIX)" != "x" ]; then \
	 install -d $(DESTDIR)$(NOKIA770_PREFIX)/share/pixmaps; \
	 cp data/images/icon.png $(DESTDIR)$(NOKIA770_PREFIX)/share/pixmaps/tuxpaint.png; \
	 chmod 644 $(DESTDIR)$(NOKIA770_PREFIX)/share/pixmaps/tuxpaint.png; \
	 cp hildon/tuxpaint.xpm $(DESTDIR)/$(NOKIA770_PREFIX)/share/pixmaps/tuxpaint.xpm; \
	 chmod 644 $(DESTDIR)$(NOKIA770_PREFIX)/share/pixmaps/tuxpaint.xpm; \
	 install -d $(DESTDIR)$(NOKIA770_PREFIX)/share/applications/hildon; \
	 cp hildon/tuxpaint.desktop $(DESTDIR)$(NOKIA770_PREFIX)/share/applications/hildon/; \
	 chmod 644 $(DESTDIR)$(NOKIA770_PREFIX)/share/applications/hildon/tuxpaint.desktop; \
	 install -d $(DESTDIR)/etc/tuxpaint; \
	 cp hildon/tuxpaint.conf $(DESTDIR)/etc/tuxpaint; \
	 chmod 644 $(DESTDIR)/etc/tuxpaint/tuxpaint.conf; \
	 rm -rf $(DESTDIR)$(NOKIA770_PREFIX)/X11R6; \
	 rm -rf $(DESTDIR)$(NOKIA770_PREFIX)/share/doc; \
	 rm -rf $(DESTDIR)$(NOKIA770_PREFIX)/share/man; \
	fi
	@-find $(DESTDIR)$(NOKIA770_PREFIX) -name CVS -type d -exec rm -rf \{\} \;



# Install a launcher icon in the KDE menu...
.PHONY: install-kde
install-kde:
	@echo
	@echo "...Installing launcher icon into KDE..."
	@if [ "x$(KDE_PREFIX)" != "x" ]; then \
	  install -d $(DESTDIR)$(KDE_PREFIX)/Graphics; \
	  cp src/tuxpaint.desktop $(DESTDIR)$(KDE_PREFIX)/Graphics/; \
	  chmod 644 $(DESTDIR)$(KDE_PREFIX)/Graphics/tuxpaint.desktop; \
	fi

.PHONY: install-kde-icons
install-kde-icons:
	@echo "...Installing launcher icon graphics into KDE..."
	@if [ "x$(KDE_ICON_PREFIX)" != "x" ]; then \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/scalable/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/192x192/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/128x128/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/96x96/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/64x64/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/48x48/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/32x32/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/22x22/apps/; \
	  install -d $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/16x16/apps/; \
	  cp data/images/tuxpaint-icon.svg \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/scalable/apps/tuxpaint.svg; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/scalable/apps/tuxpaint.svg; \
	  cp data/images/icon192x192.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/192x192/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/192x192/apps/tuxpaint.png; \
	  cp data/images/icon128x128.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/128x128/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/128x128/apps/tuxpaint.png; \
	  cp data/images/icon96x96.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/96x96/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/96x96/apps/tuxpaint.png; \
	  cp data/images/icon64x64.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/64x64/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/64x64/apps/tuxpaint.png; \
	  cp data/images/icon48x48.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/48x48/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/48x48/apps/tuxpaint.png; \
	  cp data/images/icon32x32.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/32x32/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/32x32/apps/tuxpaint.png; \
	  cp data/images/icon22x22.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/22x22/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/22x22/apps/tuxpaint.png; \
	  cp data/images/icon16x16.png \
		$(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/16x16/apps/tuxpaint.png; \
          chmod 644 $(DESTDIR)$(KDE_ICON_PREFIX)/hicolor/16x16/apps/tuxpaint.png; \
	fi


# Install the PNG icon (for GNOME, KDE, etc.)
# and the 24-color 32x32 XPM (for other Window managers):

# FIXME: Should this also use $(DESTDIR)?
.PHONY: install-icon
install-icon:
	@echo
	@echo "...Installing launcher icon graphics..."
	@install -d $(ICON_PREFIX)
	@cp data/images/icon.png $(ICON_PREFIX)/tuxpaint.png
	@chmod 644 $(ICON_PREFIX)/tuxpaint.png
	@install -d $(X11_ICON_PREFIX)
	@cp data/images/icon32x32.xpm $(X11_ICON_PREFIX)/tuxpaint.xpm
	@chmod 644 $(X11_ICON_PREFIX)/tuxpaint.xpm


# Install the program:
.PHONY: install-bin
install-bin:
	@echo
	@echo "...Installing program itself..."
	@install -d $(BIN_PREFIX)
	@cp tuxpaint$(EXE_EXT) $(BIN_PREFIX)
	@chmod a+rx,g-w,o-w $(BIN_PREFIX)/tuxpaint$(EXE_EXT)

# Install the required Windows DLLs into the 'bdist' directory
.PHONY: install-dlls
install-dlls:
	@echo
	@echo "...Installing Windows DLLs..."
	@install -d $(BIN_PREFIX)
	@cp `which tuxpaint-config.exe` $(BIN_PREFIX)
	@cp `which libintl-8.dll` $(BIN_PREFIX)
	@cp `which libiconv-2.dll` $(BIN_PREFIX)
	@cp `which libpng12.dll` $(BIN_PREFIX)
	@cp `which SDL.dll` $(BIN_PREFIX)
	@cp `which SDL_image.dll` $(BIN_PREFIX)
	@cp `which SDL_mixer.dll` $(BIN_PREFIX)
	@cp `which SDL_ttf.dll` $(BIN_PREFIX)
	@cp `which libfreetype-6.dll` $(BIN_PREFIX)
	@cp `which zlib1.dll` $(BIN_PREFIX)
	@cp `which libogg-0.dll` $(BIN_PREFIX)
	@cp `which libvorbis-0.dll` $(BIN_PREFIX)
	@cp `which libvorbisfile-3.dll` $(BIN_PREFIX)
	@cp `which libjpeg-8.dll` $(BIN_PREFIX)
	@cp `which libgcc_s_dw2-1.dll` $(BIN_PREFIX)
	@cp `which libstdc++-6.dll` $(BIN_PREFIX)
	@cp `which libfribidi-0.dll` $(BIN_PREFIX)
	@cp `which libpthread-2.dll` $(BIN_PREFIX)
	@if [ "x$(BDIST_WIN9X)" == "x" ]; then \
	  cp `which libxml2-2.dll` $(BIN_PREFIX); \
	  cp `which libcairo-2.dll` $(BIN_PREFIX); \
	  cp `which libfontconfig-1.dll` $(BIN_PREFIX); \
	  cp `which libSDL2_Pango-1.dll` $(BIN_PREFIX); \
	  cp `which libgobject-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which libgthread-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which librsvg-2-2.dll` $(BIN_PREFIX); \
	  cp `which libcroco-0.6-3.dll` $(BIN_PREFIX); \
	  cp `which libgdk_pixbuf-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which libglib-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which libgsf-1-114.dll` $(BIN_PREFIX); \
	  cp `which libpango-1.0-0.dll` $(BIN_PREFIX); \
	  cp `which libpangocairo-1.0-0.dll` $(BIN_PREFIX); \
	  cp `which libpangoft2-1.0-0.dll` $(BIN_PREFIX); \
	  cp `which libgmodule-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which libpangowin32-1.0-0.dll` $(BIN_PREFIX); \
	  cp `which libpixman-1-0.dll` $(BIN_PREFIX); \
	  cp `which libgio-2.0-0.dll` $(BIN_PREFIX); \
	  cp `which bz2-1.dll` $(BIN_PREFIX); \
	fi
	@strip -s $(BIN_PREFIX)/*.dll
	@if [ "x$(BDIST_WIN9X)" == "x" ]; then \
	  echo; \
	  echo "...Installing Configuration Files..."; \
	  cp -R win32/etc/ $(BIN_PREFIX); \
	  echo; \
	  echo "...Installing Library Modules..."; \
	  mkdir -p $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders; \
	  cp /usr/local/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders; \
	  strip -s $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; \
	  mkdir -p $(BIN_PREFIX)/lib/gtk-2.0/2.10.0/loaders; \
	  cp /usr/local/lib/gtk-2.0/loaders/*.dll $(BIN_PREFIX)/lib/gtk-2.0/2.10.0/loaders; \
	  strip -s $(BIN_PREFIX)/lib/gtk-2.0/2.10.0/loaders/*.dll; \
	  mkdir -p $(BIN_PREFIX)/lib/pango/1.6.0/modules; \
	  cp /usr/local/lib/pango/1.6.0/modules/*.dll $(BIN_PREFIX)/lib/pango/1.6.0/modules; \
	  strip -s $(BIN_PREFIX)/lib/pango/1.6.0/modules/*.dll; \
	fi

# Install symlink:
.PHONY: install-haiku
install-haiku:
	@echo
	@echo "...Installing symlink in apps/TuxPaint to tuxpaint executable file..."
	@ln -sf $(DESTDIR)$(shell finddir B_APPS_DIRECTORY)/TuxPaint/bin/tuxpaint $(DESTDIR)$(shell finddir B_APPS_DIRECTORY)/TuxPaint/tuxpaint

# Install the import script:
.PHONY: install-importscript
install-importscript:
	@echo
	@echo "...Installing 'tuxpaint-import' script..."
	@cp src/tuxpaint-import.sh $(BIN_PREFIX)/tuxpaint-import
	@chmod a+rx,g-w,o-w $(BIN_PREFIX)/tuxpaint-import


# Install the data (sound, graphics, fonts):
.PHONY: install-data
install-data:
	@echo
	@echo "...Installing data files..."
	@install -d $(DATA_PREFIX)
	@cp -R data/* $(DATA_PREFIX)
	@chmod -R a+rX,g-w,o-w $(DATA_PREFIX)
	@echo
	@echo "...Installing fonts..."
	@install -d $(DATA_PREFIX)/fonts/locale
	@cp -R fonts/locale/* $(DATA_PREFIX)/fonts/locale
	@chmod -R a+rX,g-w,o-w $(DATA_PREFIX)/fonts/locale


# Install the onscreen keyboard:
.PHONY: install-osk
install-osk:
	@echo
	@echo "...Installing onscreen keyboard files..."
	@install -d $(DATA_PREFIX)/osk
	@cp -R osk/[a-z]* $(DATA_PREFIX)/osk
	@chmod -R a+rX,g-w,o-w $(DATA_PREFIX)


# Install the text documentation:
.PHONY: install-doc
install-doc:
	@echo
	@echo "...Installing documentation..."
	@install -d $(DOC_PREFIX)
	@cp -R docs/* $(DOC_PREFIX)
	@cp -R magic/magic-docs $(DOC_PREFIX)
	@chmod -R a=rX,g=rX,u=rwX $(DOC_PREFIX)


# Install the man page:
.PHONY: install-man
install-man:
	@echo
	@echo "...Installing man pages..."
	@# man1 directory...
	@install -d $(MAN_PREFIX)/man1
	@# tuxpaint.1
	@cp src/manpage/tuxpaint.1 $(MAN_PREFIX)/man1
	@gzip -f $(MAN_PREFIX)/man1/tuxpaint.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tuxpaint.1.gz
	@# pl/man1 directory...
	@install -d $(MAN_PREFIX)/pl/man1/
	@# tuxpaint-pl.1
	@cp src/manpage/tuxpaint-pl.1 $(MAN_PREFIX)/pl/man1/tuxpaint.1
	@gzip -f $(MAN_PREFIX)/pl/man1/tuxpaint.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/pl/man1/tuxpaint.1.gz
	@# tuxpaint-import.1
	@cp src/manpage/tuxpaint-import.1 $(MAN_PREFIX)/man1/
	@gzip -f $(MAN_PREFIX)/man1/tuxpaint-import.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tuxpaint-import.1.gz
	@# tp-magic-config.1
	@cp src/manpage/tp-magic-config.1 $(MAN_PREFIX)/man1/
	@gzip -f $(MAN_PREFIX)/man1/tp-magic-config.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tp-magic-config.1.gz



# Build the program!

tuxpaint:	obj/tuxpaint.o obj/i18n.o obj/im.o obj/cursor.o obj/pixels.o \
		obj/rgblinear.o obj/playsound.o obj/fonts.o obj/parse.o \
		obj/progressbar.o obj/dirwalk.o obj/get_fname.o obj/onscreen_keyboard.o\
		$(ARCH_LIBS)
	@echo
	@echo "...Linking Tux Paint..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(FRIBIDI_CFLAGS) $(DEFS) \
		-o tuxpaint $^ \
		$(SDL_LIBS) $(SVG_LIB) $(ARCH_LINKS)
	@$(RSRC_CMD)
	@$(MIMESET_CMD)


# Build the object for the program!

obj/tuxpaint.o:	src/tuxpaint.c \
		src/i18n.h src/im.h src/cursor.h src/pixels.h \
		src/rgblinear.h src/playsound.h src/fonts.h \
		src/progressbar.h src/dirwalk.h src/get_fname.h \
		src/compiler.h src/debug.h \
		src/tools.h src/titles.h src/colors.h src/shapes.h \
		src/sounds.h src/tip_tux.h src/great.h \
		src/tp_magic_api.h src/parse.h src/onscreen_keyboard.h \
		src/$(MOUSEDIR)/arrow.xbm src/$(MOUSEDIR)/arrow-mask.xbm \
		src/$(MOUSEDIR)/hand.xbm src/$(MOUSEDIR)/hand-mask.xbm \
		src/$(MOUSEDIR)/insertion.xbm \
		src/$(MOUSEDIR)/insertion-mask.xbm \
		src/$(MOUSEDIR)/wand.xbm src/$(MOUSEDIR)/wand-mask.xbm \
		src/$(MOUSEDIR)/brush.xbm src/$(MOUSEDIR)/brush-mask.xbm \
		src/$(MOUSEDIR)/crosshair.xbm \
		src/$(MOUSEDIR)/crosshair-mask.xbm \
		src/$(MOUSEDIR)/rotate.xbm src/$(MOUSEDIR)/rotate-mask.xbm \
		src/$(MOUSEDIR)/tiny.xbm src/$(MOUSEDIR)/tiny-mask.xbm \
		src/$(MOUSEDIR)/watch.xbm src/$(MOUSEDIR)/watch-mask.xbm \
		src/$(MOUSEDIR)/up.xbm src/$(MOUSEDIR)/up-mask.xbm \
		src/$(MOUSEDIR)/down.xbm src/$(MOUSEDIR)/down-mask.xbm \
		$(ARCH_HEADERS) \
		Makefile
	@echo
	@echo "...Compiling Tux Paint from source..."
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(FRIBIDI_CFLAGS) $(SVG_CFLAGS) $(MOUSE_CFLAGS) $(DEFS) \
		-c src/tuxpaint.c -o obj/tuxpaint.o

# Broke gperf|sed up into two steps so that it will fail properly if gperf is not installed; there's probably a more elegant solution -bjk 2009.11.20
obj/parse.c:	obj/parse_step1.c
	@echo
	@echo "...Generating the command-line and config file parser (STEP 2)..."
	@sed -r -e 's/^const struct/static const struct/' -e 's/_GNU/_TUX/' obj/parse_step1.c > obj/parse.c
	
obj/parse_step1.c:	src/parse.gperf
	@echo
	@echo "...Generating the command-line and config file parser (STEP 1)..."
	@if [ -x $(GPERF) ] ; then \
		$(GPERF) src/parse.gperf > obj/parse_step1.c ; \
	else \
		echo "Please install 'gperf' and try again!" ; \
		false ; \
	fi

obj/parse.o:	obj/parse.c src/parse.h src/compiler.h
	@echo
	@echo "...Compiling the command-line and config file parser..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) \
		-c obj/parse.c -o obj/parse.o

obj/i18n.o:	src/i18n.c src/i18n.h src/debug.h
	@echo
	@echo "...Compiling i18n support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) \
		-c src/i18n.c -o obj/i18n.o

obj/im.o:	src/im.c src/im.h src/debug.h
	@echo
	@echo "...Compiling IM support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/im.c -o obj/im.o

obj/get_fname.o:	src/get_fname.c src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling filename support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) \
		-c src/get_fname.c -o obj/get_fname.o

obj/fonts.o:	src/fonts.c src/fonts.h src/dirwalk.h src/progressbar.h \
		src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling font support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/fonts.c -o obj/fonts.o

obj/dirwalk.o:	src/dirwalk.c src/dirwalk.h src/progressbar.h src/fonts.h \
		src/debug.h
	@echo
	@echo "...Compiling directory-walking support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/dirwalk.c -o obj/dirwalk.o

obj/cursor.o:	src/cursor.c src/cursor.h src/debug.h
	@echo
	@echo "...Compiling cursor support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(MOUSE_CFLAGS) $(DEFS) \
		-c src/cursor.c -o obj/cursor.o

obj/pixels.o:	src/pixels.c src/pixels.h src/compiler.h src/debug.h
	@echo
	@echo "...Compiling pixel functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/pixels.c -o obj/pixels.o

obj/playsound.o:	src/playsound.c src/playsound.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling sound playback functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/playsound.c -o obj/playsound.o

obj/progressbar.o:	src/progressbar.c src/progressbar.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling progress bar functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/progressbar.c -o obj/progressbar.o

obj/rgblinear.o:	src/rgblinear.c src/rgblinear.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling RGB to Linear functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/rgblinear.c -o obj/rgblinear.o


obj/BeOS_print.o:	src/BeOS_print.cpp src/BeOS_print.h
	@echo
	@echo "...Compiling BeOS print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/BeOS_print.cpp -o obj/BeOS_print.o

obj/win32_print.o:	src/win32_print.c src/win32_print.h src/debug.h
	@echo
	@echo "...Compiling win32 print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/win32_print.c -o obj/win32_print.o

obj/postscript_print.o:	src/postscript_print.c Makefile \
			src/postscript_print.h src/debug.h
	@echo
	@echo "...Compiling PostScript print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/postscript_print.c -o obj/postscript_print.o

obj/resource.o:	win32/resources.rc win32/resource.h
	@echo
	@echo "...Compiling win32 resources..."
	@$(WINDRES) -i win32/resources.rc -o obj/resource.o

obj/onscreen_keyboard.o:	src/onscreen_keyboard.c src/onscreen_keyboard.h src/dirwalk.h src/progressbar.h \
		src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling on screen keyboard support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) \
		-c src/onscreen_keyboard.c -o obj/onscreen_keyboard.o

src/tp_magic_api.h:	src/tp_magic_api.h.in
	@echo
	@echo "...Generating 'Magic' tool API development header file..."
	@(echo "/*\n\n\n\n\n\n\n\nDO NOT EDIT ME!\n\n\n\n\n\n\n\n*/" ; cat src/tp_magic_api.h.in) | sed -e s/__APIVERSION__/$(MAGIC_API_VERSION)/ > src/tp_magic_api.h


tp-magic-config:	src/tp-magic-config.sh.in Makefile
	@echo
	@echo "...Generating 'Magic' tool API configuration script..."
	@sed -e s/__VERSION__/$(VER_VERSION)/ \
		-e s/__APIVERSION__/$(MAGIC_API_VERSION)/ \
		-e s=__INCLUDE__=$(INCLUDE_PREFIX)/tuxpaint= \
		-e s=__DATAPREFIX__=$(DATA_PREFIX)= \
		-e s=__PLUGINPREFIX__=$(MAGIC_PREFIX)= \
		-e s=__PLUGINDOCPREFIX__=$(DOC_PREFIX)/magic-docs= \
		src/tp-magic-config.sh.in \
		> tp-magic-config

# Make the "obj" directory to throw the object(s) into:
# (not necessary any more; bjk 2006.02.20)

obj:
	@mkdir obj

######

MAGIC_SDL_CPPFLAGS:=$(shell $(PKG_CONFIG) $(SDL_PCNAME) --cflags)
MAGIC_SDL_LIBS:=-L/usr/local/lib $(LIBMINGW) $(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL_image -lSDL_ttf $(SDL_MIXER_LIB)
MAGIC_ARCH_LINKS:=-lintl $(PNG)

windows_PLUGIN_LIBS:=$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS)
osx_PLUGIN_LIBS:=
beos_PLUGIN_LIBS:="$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS) $(MAGIC_SDL_CPPFLAGS)"
linux_PLUGIN_LIBS:=
PLUGIN_LIBS:=$($(OS)_PLUGIN_LIBS)

#MAGIC_CFLAGS:=-g3 -O2 -fvisibility=hidden -fno-common -W -Wstrict-prototypes -Wmissing-prototypes -Wall $(MAGIC_SDL_CPPFLAGS) -Isrc/
MAGIC_CFLAGS:=-g3 -O2 -fno-common -W -Wstrict-prototypes -Wmissing-prototypes -Wall $(MAGIC_SDL_CPPFLAGS) -Isrc/
SHARED_FLAGS:=-shared -fpic -Wl,--warn-shared-textrel

MAGIC_C:=$(wildcard magic/src/*.c)
MAGIC_SO:=$(patsubst magic/src/%.c,magic/%.$(SO_TYPE),$(MAGIC_C))

$(MAGIC_SO): magic/%.$(SO_TYPE): magic/src/%.c  
	$(CC) $(MAGIC_CFLAGS) $(LDFLAGS) $(SHARED_FLAGS) -o $@ $< $(PLUGIN_LIBS)
# Probably should separate the various flags like the following:
#	$(CC) $(PLUG_CPPFLAGS) $(PLUG_CFLAGS) $(PLUG_LDFLAGS) -o $@ $< $(PLUG_LIBS)

.PHONY: magic-plugins
magic-plugins:	src/tp_magic_api.h $(MAGIC_SO)
