# Tux Paint - A simple drawing program for children.

# Copyright (c) 2002-2023
# Various contributors (see AUTHORS.txt)
# https://tuxpaint.org/

# June 14, 2002 - January 25, 2023


# The version number, for release:

VER_VERSION:=0.9.29
VER_FLAVOR:="-sdl2"
ifdef SOURCE_DATE_EPOCH
  VER_DATE=$(shell date -u -d "@$(SOURCE_DATE_EPOCH)" "+%Y-%m-%d" 2>/dev/null || date -u -r "$(SOURCE_DATE_EPOCH)" "+%Y-%m-%d" 2>/dev/null || date -u "+%Y-%m-%d")
else
  VER_DATE=$(shell date "+%Y-%m-%d")
endif
MAGIC_API_VERSION:=0x00000007

# Need to know the OS

SYSNAME:=$(shell uname -s)
ifeq ($(findstring MINGW32, $(SYSNAME)),MINGW32)
  OS:=windows
  GPERF:=/usr/bin/gperf
  MINGW_DIR:=/mingw32
else
  ifeq ($(findstring MINGW64, $(SYSNAME)),MINGW64)
    OS:=windows
    GPERF:=/usr/bin/gperf
    MINGW_DIR:=/mingw64
  else
    ifeq ($(SYSNAME),Darwin)
      OS:=macos
      GPERF:=/usr/bin/gperf

      CC:=$(shell xcrun -f clang)
      ARCHS:=$(shell uname -m)
      MINVER:=10.8
      SDKROOT:=$(shell xcrun --show-sdk-path)
      HOSTROOT:=/opt/local
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
endif

# CROSS COMPILATION OVERRIDES
#
# Usage:
#   make HOST=<HOST> HOSTROOT=<HOSTROOT> [EXTRA_ARGS]
#
# <HOST> may be one of:
#   iphoneos          Build for the iphoneos, presumably on macOS.
#   iphonesimulator   Build for the iphonesimulator, presumably on macOS.
#
# <HOSTROOT> is the directory containing support files for building for <HOST>:
#   <HOSTROOT>/include        Header files.
#   <HOSTROOT>/lib            Library files.
#   <HOSTROOT>/lib/pkgconfig  *.pc files.
#
ifdef HOST
  ifdef HOSTROOT
    ifeq ($(wildcard $(HOSTROOT)/.),)
      $(error Invalid HOSTROOT: $(HOSTROOT))
    endif

    ifeq ($(HOST),iphoneos)
      OS:=ios
      CC:=$(shell xcrun --sdk iphoneos -f clang)
      SDK:=iphoneos
      ARCHS:=arm64 armv7s armv7
      MINVER:=9.0
      MINVEROPT:=-miphoneos-version-min=$(MINVER)
      SDKROOT:=$(shell xcrun --sdk iphoneos --show-sdk-path)
    else ifeq ($(HOST),iphonesimulator)
      OS:=ios
      CC:=$(shell xcrun --sdk iphonesimulator -f clang)
      SDK:=iphonesimulator
      ARCHS:=$(shell uname -m)
      MINVER:=9.0
      MINVEROPT:=-mios-simulator-version-min=$(MINVER)
      SDKROOT:=$(shell xcrun --sdk iphonesimulator --show-sdk-path)
    else
      $(error Invalid HOST for cross compilation: $(HOST))
    endif

    # We set PKG_CONFIG_LIBDIR instead of PKG_CONFIG_PATH because we want to
    # *change* where pkg-config looks for .pc files instead of adding to the
    # default path which may have libraries that aren't for HOST.
    export PKG_CONFIG_LIBDIR:=$(HOSTROOT)/lib/pkgconfig
  endif
endif

# change to sdl-console to build a console version on Windows
SDL_PCNAME:=sdl2

WINDRES:=windres
PKG_CONFIG:=pkg-config
ifdef PKG_CONFIG_LIBDIR
  # Cross compilation override
  PKG_CONFIG:=PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) $(PKG_CONFIG)
endif

# test if a pkg-config library exists or can be linked manually
linktest = $(shell [ -n "$(1)" ] \
    && $(PKG_CONFIG) --exists $(1) \
    && $(PKG_CONFIG) --libs $(1) \
    || if $(CC) $(CPPFLAGS) $(CFLAGS) -o dummy.o dummy.c $(LDFLAGS) $(2) $(3) > /dev/null 2>&1; \
	then \
		echo "$(2)"; \
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

macos_RAD_CMD:=[[ ! -d Resources/share ]] && mkdir -p Resources/share && ln -s ../../data Resources/share/tuxpaint || :
RAD_CMD:=$($(OS)_RAD_CMD)

windows_SO_TYPE:=dll
macos_SO_TYPE:=dylib
ios_SO_TYPE:=dylib
beos_SO_TYPE:=so
linux_SO_TYPE:=so
SO_TYPE:=$($(OS)_SO_TYPE)

windows_LIBMINGW:=-L/usr/local/lib -lmingw32
LIBMINGW:=$($(OS)_LIBMINGW)

windows_EXE_EXT:=.exe
EXE_EXT:=$($(OS)_EXE_EXT)

macos_BUNDLE:=./TuxPaint.app
ios_BUNDLE:=./TuxPaint-$(SDK).app
BUNDLE:=$($(OS)_BUNDLE)

windows_ARCH_LIBS:=obj/win32_print.o obj/resource.o obj/win32_trash.o
macos_ARCH_LIBS:=src/macos_print.m obj/macos.o
ios_ARCH_LIBS:=src/ios_print.m obj/ios.o
beos_ARCH_LIBS:=obj/BeOS_print.o
linux_ARCH_LIBS:=obj/postscript_print.o
ARCH_LIBS:=$($(OS)_ARCH_LIBS)

windows_ARCH_CFLAGS:=
macos_ARCH_CFLAGS:=-isysroot $(SDKROOT) -I$(SDKROOT)/usr/include -I$(HOSTROOT)/include -mmacosx-version-min=$(MINVER) -arch $(subst $() $(), -arch ,$(ARCHS)) -w -headerpad_max_install_names -DHAVE_STRCASESTR
ios_ARCH_CFLAGS:=-isysroot $(SDKROOT) -I$(SDKROOT)/usr/include -I$(HOSTROOT)/include $(MINVEROPT) -arch $(subst $() $(), -arch ,$(ARCHS)) -w -fPIC -DHAVE_STRCASESTR -DUNLINK_ONLY
beos_ARCH_CFLAGS:=
linux_ARCH_CFLAGS:=
ARCH_CFLAGS:=$($(OS)_ARCH_CFLAGS)

windows_ARCH_LDFLAGS:=
macos_ARCH_LDFLAGS:=-isysroot $(SDKROOT) -L$(HOSTROOT)/lib -mmacosx-version-min=$(MINVER) -arch $(subst $() $(), -arch ,$(ARCHS))
ios_ARCH_LDFLAGS:=-isysroot $(SDKROOT) -L$(HOSTROOT)/lib $(MINVEROPT) -arch $(subst $() $(), -arch ,$(ARCHS))
beos_ARCH_LDFLAGS:=
linux_ARCH_LDFLAGS:=
ARCH_LDFLAGS:=$($(OS)_ARCH_LDFLAGS)
LDFLAGS:=$(ARCH_LDFLAGS)

PAPER_LIB:=$(call linktest,,-lpaper,)
PNG:=$(call linktest,libpng,-lpng,)
PNG:=$(if $(PNG),$(PNG),$(call linktest,,-lpng12,))

FRIBIDI_LIB:=$(shell $(PKG_CONFIG) --libs fribidi)
FRIBIDI_CFLAGS:=$(shell $(PKG_CONFIG) --cflags fribidi)

windows_ARCH_LINKS:=-lgdi32 -lcomdlg32 $(PNG) -lz -lwinspool -lshlwapi $(FRIBIDI_LIB) -liconv -limagequant -mwindows
macos_ARCH_LINKS:=$(FRIBIDI_LIB) -limagequant -lSDLmain -Wl,-framework,AppKit -Wl,-framework,Cocoa $(shell $(PKG_CONFIG) --libs pangoft2)
ios_ARCH_LINKS=$(FRIBIDI_LIB) -limagequant -ljpeg -lbz2 $(shell $(PKG_CONFIG) --libs freetype2 libtiff-4 libwebp libffi harfbuzz libmpg123 ogg vorbisenc vorbisidec libxml-2.0 pangoft2 libpcre)
beos_ARCH_LINKS:=-lintl $(PNG) -lz -lbe -lnetwork -liconv $(FRIBIDI_LIB) $(PAPER_LIB) $(STDC_LIB) -limagequant
linux_ARCH_LINKS:=$(PAPER_LIB) $(FRIBIDI_LIB) -limagequant
ARCH_LINKS:=$($(OS)_ARCH_LINKS)

windows_ARCH_HEADERS:=src/win32_print.h
macos_ARCH_HEADERS:=src/macos.h
beos_ARCH_HEADERS:=src/BeOS_print.h
linux_ARCH_HEADERS:=
ARCH_HEADERS:=$($(OS)_ARCH_HEADERS)

# Where things will go when ultimately installed:
# For macOS and iOS, the prefix is relative to DESTDIR.
windows_PREFIX:=/usr/local
macos_PREFIX:=Resources
ios_PREFIX:=.
linux_PREFIX:=/usr/local
PREFIX:=$($(OS)_PREFIX)

# Root directory to place files when creating packages.
# PKG_ROOT is the old name for this, and should be undefined.
# macOS and iOS are set up as bundles, with all files under the bundle.
# "TuxPaint-1" is the OLPC XO name. Installing to ./ is bad!
ifeq ($(OS),macos)
  DESTDIR:=$(BUNDLE)/Contents/
else ifeq ($(OS),ios)
  DESTDIR:=$(BUNDLE)/
else ifeq ($(PREFIX),./)
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
MAGIC_PREFIX:=$(DESTDIR)$(LIBDIR)/lib$(LIBDIRSUFFIX)/tuxpaint/plugins

# Docs and man page:
DOC_PREFIX:=$(DESTDIR)$(PREFIX)/share/doc/tuxpaint-$(VER_VERSION)
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
X11_ICON_PREFIX:=$(DESTDIR)$(PREFIX)/share/pixmaps

# Appstream metainfo
METAINFO_PREFIX=$(DESTDIR)$(PREFIX)/share/metainfo

# Maemo flag
MAEMOFLAG:=


# Where to find cursor shape XBMs

MOUSEDIR:=mouse
CURSOR_SHAPES:=LARGE
# MOUSEDIR:=mouse/16x16
# CURSOR_SHAPES:=SMALL

# Libraries, paths, and flags:
SDL_LIBS:=$(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs)
SDL_LIBS+=$(call linktest,SDL2_image,-lSDL2_image,$(SDL_LIBS))
SDL_LIBS+=$(call linktest,SDL2_ttf,-lSDL2_ttf,$(SDL_LIBS))
SDL_LIBS+=$(call linktest,SDL2_gfx,-lSDL2_gfx,$(SDL_LIBS))
SDL_LIBS+=$(call linktest,zlib,-lz,)
SDL_LIBS+=$(call linktest,libpng,$(PNG),)

# Sound support
SDL_MIXER_LIB:=$(call linktest,SDL2_mixer,-lSDL2_mixer,$(SDL_LIBS))
NOSOUNDFLAG:=$(if $(SDL_MIXER_LIB),,-DNOSOUND$(warning -lSDL2_Mixer failed, no sound for you!))

# SDL Pango is needed to render complex scripts like Thai and Arabic
SDL2_PANGO_LIB:=$(call linktest,SDL2_Pango,-lSDL2_Pango,$(SDL_LIBS))
SDL2_PANGO_CFLAGS:=$(shell $(PKG_CONFIG) --cflags SDL2_Pango)
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


PNG_CFLAGS:=$(shell $(PKG_CONFIG) libpng --cflags)


ifeq ($(hack),1)
hack:
	@echo 'SDL2_PANGO_LIB is' $(SDL2_PANGO_LIB)
	@echo 'SDL_MIXER_LIB is' $(SDL_MIXER_LIB)
	@echo 'SVG_LIB       is' $(SVG_LIB)
	@echo 'SDL_LIBS      is' $(SDL_LIBS)
	@echo 'SDL_CFLAGS    is' $(SDL_CFLAGS)
	@echo 'SVG_CFLAGS    is' $(SVG_CFLAGS)
	@echo 'PAPER_LIB     is' $(PAPER_LIB)
	@echo 'PNG           is' $(PNG)
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
	$(shell src/test-option.sh -Wstrict-aliasing=2) \
    $(SDL2_PANGO_CFLAGS) \
	$(ARCH_CFLAGS)

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

# On an average-sized screen (e.g., 800x600 window), the thumbnails
# are 132x80.  On larger screens, they will be bigger (since the New dialog
# is always 4x4 thumbnails); therefore, generating larger thumbs, which can
# be still be scaled down fairly quickly (esp. complicated SVG ones).
CONVERT_OPTS:=-alpha Background -alpha Off +depth -resize !264x160 -background white -interlace none

.SUFFIXES:

#############################################################################
#############################################################################
#############################################################################
#
# "make" with no arguments builds the program and man page from sources:
#
.PHONY: all
all:	tuxpaint translations magic-plugins tp-magic-config thumb-starters thumb-templates
	@echo
	@echo "--------------------------------------------------------------"
	@echo
	@echo "Done compiling."
	@echo
	@if [ "x$(OS)" = "xmacos" ]; then \
	    echo "Now run 'make install' to create the App Bundle ($(BUNDLE))."; \
        echo; \
    else \
	    echo "Now run 'make install' with any options you ran 'make' with."; \
	    echo "to install Tux Paint."; \
	    echo; \
	    echo "You may need superuser ('root') privileges, depending on"; \
	    echo "where you're installing."; \
	    echo "(Depending on your system, you either need to 'su' first,"; \
	    echo "or run 'sudo make install'.)"; \
	    echo; \
    fi

.PHONY: releaseclean
releaseclean:
	@echo
	@echo "Cleaning release directory"
	@echo
	@rm -rf "build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR)" "build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR).tar.gz"
	@-if [ -d build ] ; then rmdir build ; fi

.PHONY: releasedir
releasedir: build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR)


build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR):
	@echo
	@echo "Creating release directory"
	@echo
	@mkdir -p build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR)
	@find . -follow \
	     \( -wholename '*/.git' -o -name .gitignore -o -name .thumbs -o -name .cvsignore -o -name 'dummy.o' -o -name 'build' -o -name '.#*' \) \
	     -prune -o -type f -exec cp --parents -vdp \{\} build/tuxpaint-$(VER_VERSION)$(VER_FLAVOR)/ \;

.PHONY: release
release: releasedir
	@echo
	@echo "Creating release tarball"
	@echo
	@cd build ; \
	    tar -czvf tuxpaint-$(VER_VERSION)$(VER_FLAVOR).tar.gz tuxpaint-$(VER_VERSION)$(VER_FLAVOR)

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
$(INSTALLED_MOFILES): $(LOCALE_PREFIX)/%/LC_MESSAGES/tuxpaint.mo: trans/%.mo $(INSTALLED_MODIRS)
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

%.desktop: %.desktop.in $(POTFILES)
	msgfmt --desktop -d src/po --template $< -o $@

%.appdata.xml: %.appdata.xml.in $(POTFILES)
	msgfmt --xml -d src/po --template $< -o $@

.PHONY: translations
ifeq "$(shell msgfmt -h)" ""
translations: trans
	@echo "--------------------------------------------------------------"
	@echo "Cannot find program 'msgfmt'!"
	@echo "No translation files will be prepared."
	@echo "Install gettext to run Tux Paint in non-U.S. English modes."
	@echo "--------------------------------------------------------------"
else
translations: trans $(MOFILES) src/tuxpaint.desktop src/org.tuxpaint.Tuxpaint.appdata.xml
endif

trans:
	@echo
	@echo "...Preparing translation files..."
	@mkdir trans

######

windows_ARCH_INSTALL:=
macos_ARCH_INSTALL:=install-macbundle
ios_ARCH_INSTALL:=install-iosbundle
linux_ARCH_INSTALL:=install-xdg install-man install-importscript install-bash-completion
ARCH_INSTALL:=$($(OS)_ARCH_INSTALL)

# "make install" installs all of the various parts
# (depending on the *PREFIX variables at the top, you probably need
# to do this as superuser ("root"))
.PHONY: install
install:	install-bin install-data install-doc \
		install-magic-plugins \
		install-magic-plugin-dev \
		install-icon install-gettext install-im \
		install-default-config install-example-stamps \
		install-example-starters install-example-templates \
		install-thumb-starters install-thumb-templates \
		install-osk \
		$(ARCH_INSTALL)
	@echo
	@echo "--------------------------------------------------------------"
	@echo
	@if [ "x$(OS)" = "xmacos" ]; then \
        echo "The App Bundle has been created as $(BUNDLE)!  Now you can:"; \
        echo; \
        echo "  * Double click $(BUNDLE) to run the application,"; \
        echo "  * sign the App Bundle (see below),"; \
        echo "  * build the universal App Bundle (see below),"; \
        echo "  * and/or run 'make TuxPaint.dmg' to create the DMG file for distribution."; \
        echo; \
        echo "For usage, see $(DOC_PREFIX)/[locale]/README.txt"; \
        echo; \
        echo "SIGNING THE APP BUNDLE"; \
        echo "----------------------"; \
        echo "Signing is optional for the Intel CPU build, or for the Apple Silicon build if"; \
        echo "it is to be run only on the system on which it was built (e.g., for"; \
        echo "development.)  The App Bundle must be signed if it is built to run natively on"; \
        echo "the Apple Silicon and is distributed."; \
        echo; \
        echo "To sign the App Bundle, use the following commands, where IDENTITY is your Apple"; \
        echo "Developer ID if you have one, or a hyphen (-) to sign it ad-hoc:"; \
        echo; \
        echo "  codesign --remove-signature $(BUNDLE)  # to remove any existing signature"; \
        echo "  codesign -s IDENTITY $(BUNDLE)"; \
        echo; \
        echo "If you are building the universal Apple Bundle, sign the App Bundle *after*"; \
        echo "building the universal App Bundle."; \
        echo; \
        echo "BUILDING THE UNIVERSAL APP BUNDLE"; \
        echo "---------------------------------"; \
        echo "Building the universal App Bundle involves building Tux Paint on the x86 machine"; \
        echo "and also on the arm64 machine, then combining the two App Bundles using the supplied"; \
        echo "script, 'macos/build-universal.sh'.  Name the two App Bundles 'TuxPaint-x86_64.app'"; \
        echo "and 'TuxPaint-arm64.app' before running the script; the universal app bundle will be"; \
        echo "named 'TuxPaint.app'."; \
        echo; \
        echo "However, if your aim is to build a portable App Bundle, some extra effort will be needed."; \
        echo "to make the code work on older versions of macOS than on those in which the bundles were"; \
        echo "built.  There are a few options on how to do this.  Please see the details in:"; \
        echo; \
        echo "  $(DOC_PREFIX)/[locale]/INSTALL.txt"; \
	else \
		echo "All done! Now (preferably NOT as 'root' superuser),"; \
		echo "you can type the command 'tuxpaint' to run the program!!!"; \
		echo; \
		echo "For more information, see the 'tuxpaint' man page,"; \
		echo "run 'tuxpaint --usage' or see $(DOC_PREFIX)/README.txt"; \
	fi
	@echo
	@echo "Visit Tux Paint's home page for more information, updates"
	@echo "and to learn how you can help out!"
	@echo
	@echo "  https://tuxpaint.org/"
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
install-magic-plugin-dev:	src/tp_magic_api.h install-bin
	@echo
	@echo "...Installing Magic Tool plug-in development files and docs..."
	@cp tp-magic-config $(BIN_PREFIX)
	@chmod a+rx,g-w,o-w $(BIN_PREFIX)/tp-magic-config
	@install -d $(INCLUDE_PREFIX)/tuxpaint
	@cp src/tp_magic_api.h $(INCLUDE_PREFIX)/tuxpaint
	@chmod a+r,g-w,o-w $(INCLUDE_PREFIX)/tuxpaint/tp_magic_api.h

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
		IM_PREFIX:=im \
		CONFDIR:=. \
		COMPLETIONDIR:=. \
		INCLUDE_PREFIX:=plugins/include \
		MAGIC_PREFIX:=plugins \
		ARCH_DEFS:=-DBDIST_WIN32
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
		ARCH_INSTALL:=install-dlls install-tpconf-i18n
	mv ./win32/bdist/tuxpaint.conf ./win32/bdist/tuxpaint.cfg
	unix2dos ./win32/bdist/tuxpaint.cfg

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
	@-if [ "x$(BUNDLE)" != "x" ]; then rm -rf $(BUNDLE); fi
	@-rm -f TuxPaint.dmg temp.dmg {macos,ios}/Info.plist; rm -rf magic/*.dSYM Resources
	@-rm -f dlllist a.exe
	@-rm -f win32/Preprocessed.iss win32/tuxpaint-*.zip win32/tuxpaint-*.exe
	@-rm -f test-png
	@echo

# "make uninstall" should remove the various parts from their
# installation locations.  BE SURE the *PREFIX variables at the top
# are the same as they were when you installed, of course!!!
.PHONY: uninstall
uninstall:	uninstall-i18n
	-rm $(METAINFO_PREFIX)/org.tuxpaint.Tuxpaint.appdata.xml
	-rm /usr/share/applications/tuxpaint.desktop
	-rm /usr/share/pixmaps/tuxpaint.png
	-rm $(ICON_PREFIX)/tuxpaint.png
	-rm $(X11_ICON_PREFIX)/tuxpaint.xpm
	-rm $(BIN_PREFIX)/tuxpaint
	-rm $(BIN_PREFIX)/tuxpaint-import
	-rm -r $(DATA_PREFIX)
	-rm -r $(DOC_PREFIX)
	-rm $(MAN_PREFIX)/man1/tuxpaint.1.gz
	-rm $(MAN_PREFIX)/*/man1/tuxpaint.1.gz
	-rm $(MAN_PREFIX)/man1/tuxpaint-import.1.gz
	-rm $(MAN_PREFIX)/*/man1/tuxpaint-import.1.gz
	-rm $(MAN_PREFIX)/man1/tp-magic-config.1.gz
	-rm $(MAN_PREFIX)/*/man1/tp-magic-config.1.gz
	-rm -f -r $(CONFDIR)
	-rm $(COMPLETIONDIR)/tuxpaint-completion.bash
	-rm -r $(MAGIC_PREFIX)
	-rm -r $(INCLUDE_PREFIX)/tuxpaint
	-rm $(BIN_PREFIX)/tp-magic-config
	-if [ "x$(BUNDLE)" != "x" ]; then \
	  rm -rf $(BUNDLE); \
	fi
	@if [ "x$(shell which xdg-icon-resource install)" != "x" ]; then \
	  xdg-icon-resource uninstall --size 192 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 128 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 96 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 64 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 48 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 32 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 22 tux4kids-tuxpaint ; \
	  xdg-icon-resource uninstall --size 16 tux4kids-tuxpaint ; \
	fi
	@if [ "x$(shell which xdg-desktop-menu)" != "x" ]; then \
	  xdg-desktop-menu uninstall tux4kids-tuxpaint.desktop ; \
	fi
	-if [ "x$(shell which update-desktop-database)" != "x" ]; then \
	  update-desktop-database; \
	fi

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

$(INSTALLED_STARTERS): $(DATA_PREFIX)/%: % install-example-starters-dirs
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
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=.jpg))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=.jpeg))))

STARTER_BACK_NAME=$(or $(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.svg))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.png))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.jpg))),\
		$(wildcard $(subst starters/.thumbs,starters,$(@:-t.png=-back.jpeg))))

# FIXME: Need to be able to update a thumbnail if the source image is modified -bjk 2019.09.14
$(THUMB_STARTERS):
	@printf "."
	@mkdir -p starters/.thumbs
	@if [ "x" != "x"$(STARTER_BACK_NAME) ] ; \
	then \
		composite $(STARTER_NAME) $(STARTER_BACK_NAME) obj/tmp_$(notdir $(STARTER_NAME)).png ; \
		convert $(CONVERT_OPTS) obj/tmp_$(notdir $(STARTER_NAME)).png $@ 2> /dev/null ; \
		rm obj/tmp_$(notdir $(STARTER_NAME)).png ; \
	else \
		convert $(CONVERT_OPTS) $(STARTER_NAME) $@ 2> /dev/null || ( echo "($@ failed)" ; rm $@ ) ; \
	fi

$(INSTALLED_THUMB_STARTERS): $(DATA_PREFIX)/%: % install-example-starters-dirs
	@install -D -m 644 $< $@ || ( echo "NO THUMB $<" )

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

$(INSTALLED_TEMPLATES): $(DATA_PREFIX)/%: % install-example-template-dirs
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

THUMB_TEMPLATES:=$(sort $(patsubst templates%, templates/.thumbs%-t.png, $(basename $(subst -back.,.,$(TEMPLATES)))))
INSTALLED_THUMB_TEMPLATES:=$(patsubst %,$(DATA_PREFIX)/%,$(THUMB_TEMPLATES))

TEMPLATE_NAME=$(or $(wildcard $(subst templates/.thumbs,templates,$(@:-t.png=.svg))),\
		$(wildcard $(subst templates/.thumbs,templates,$(@:-t.png=.png))),\
		$(wildcard $(subst templates/.thumbs,templates,$(@:-t.png=.jpg))),\
		$(wildcard $(subst templates/.thumbs,templates,$(@:-t.png=.jpeg))))

# FIXME: Need to be able to update a thumbnail if the source image is modified -bjk 2019.09.14
$(THUMB_TEMPLATES):
	@printf "."
	@mkdir -p templates/.thumbs
	@convert $(CONVERT_OPTS) $(TEMPLATE_NAME) $@ 2> /dev/null || ( echo "($@ failed)" ; rm $@ ) ; \

$(INSTALLED_THUMB_TEMPLATES): $(DATA_PREFIX)/%: %
	@install -D -m 644 $< $@ || ( echo "NO THUMB $<" )

.PHONY: echo-thumb-templates
echo-thumb-templates:
	@echo
	@echo "...Generating thumbnails for templates..."

# Create thumbnails for templates
.PHONY: thumb-templates
thumb-templates: echo-thumb-templates $(THUMB_TEMPLATES)

.PHONY: echo-install-thumb-templates
echo-install-thumb-templates:
	@echo
	@echo "...Installing thumbnails for templates..."

# Install thumb templates
.PHONY: install-thumb-templates
install-thumb-templates: echo-install-thumb-templates $(INSTALLED_THUMB_TEMPLATES)


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

# Install a launcher icon in the Linux desktop environment's (freedesktop.org) menus...
# FIXME: No way to install SVG icons using `xdg-icon-resource`
# (see https://bugs.launchpad.net/ubuntu/+source/xdg-utils/+bug/790449)
.PHONY: install-xdg
install-xdg: src/tuxpaint.desktop src/org.tuxpaint.Tuxpaint.appdata.xml
	@echo
	@echo "...Installing launcher icon into desktop environment..."
	@if [ "x$(shell which xdg-icon-resource)" != "x" ]; then \
	  xdg-icon-resource install --size 192 data/images/icon192x192.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 128 data/images/icon128x128.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 96 data/images/icon96x96.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 64 data/images/icon64x64.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 48 data/images/icon48x48.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 32 data/images/icon32x32.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 22 data/images/icon22x22.png tux4kids-tuxpaint ; \
	  xdg-icon-resource install --size 16 data/images/icon16x16.png tux4kids-tuxpaint ; \
	fi
	@if [ "x$(shell which xdg-desktop-menu)" != "x" ]; then \
	  cp src/tuxpaint.desktop ./tux4kids-tuxpaint.desktop ; \
	  xdg-desktop-menu install tux4kids-tuxpaint.desktop ; \
	  rm ./tux4kids-tuxpaint.desktop ; \
	fi
	@if [ "x$(shell which update-desktop-database)" != "x" ]; then \
	  update-desktop-database ; \
	fi
	install --mode=0644 -Dt $(METAINFO_PREFIX) src/org.tuxpaint.Tuxpaint.appdata.xml

# Install the PNG icon (for KDE desktop, etc.)
# and the 24-color 32x32 XPM (for other Window managers):
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

# Install tuxpaint-config and required Windows DLLs into the 'bdist' directory
.PHONY: install-dlls
install-dlls:
	@echo
	@echo "...Installing Windows DLLs..."
	@install -d $(BIN_PREFIX)
	@cp $(TPCONF_PATH)/tuxpaint-config.exe $(BIN_PREFIX)
	@src/install-dlls.sh tuxpaint.exe $(TPCONF_PATH)/tuxpaint-config.exe $(BIN_PREFIX)
	@strip -s $(BIN_PREFIX)/*.dll
	@echo
	@echo "...Installing Configuration Files..."
	@cp -R win32/etc/ $(BIN_PREFIX)
	@echo
	@echo "...Installing Library Modules..."
	@mkdir -p $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders
	@cp $(MINGW_DIR)/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders
	@strip -s $(BIN_PREFIX)/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll

# Install symlink:
.PHONY: install-haiku
install-haiku:
	@echo
	@echo "...Installing symlink in apps/TuxPaint to tuxpaint executable file..."
	@ln -sf $(DESTDIR)$(shell finddir B_APPS_DIRECTORY)/TuxPaint/bin/tuxpaint $(DESTDIR)$(shell finddir B_APPS_DIRECTORY)/TuxPaint/tuxpaint

# Install the import script:
.PHONY: install-importscript
install-importscript: install-bin
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
	@rm $(DOC_PREFIX)/Makefile # Used to generate TXT from HTML
	@rm $(DOC_PREFIX)/RELEASE.txt # Not useful to end users
	@echo
	@echo "...Installing English Magic tool docs..."
	@install -d $(DOC_PREFIX)/en/magic-docs
	@cp -R magic/magic-docs/en/* $(DOC_PREFIX)/en/magic-docs/
	@echo
	for l in `ls -d man/*.UTF-8 | cut -d '/' -f 2`; do \
		DEST=$(DOC_PREFIX)/$$l/magic-docs ; \
		echo "...Installing $$l Magic tool docs into $$DEST..." ; \
		install -d $$DEST ; \
		cp -R magic/magic-docs/$$l/* $$DEST/ ; \
	done
	@chmod -R a=rX,g=rX,u=rwX $(DOC_PREFIX)


# Install the man page:
.PHONY: install-man
install-man:
	@echo
	@echo "...Installing English man pages..."
	@# man1 directory...
	@install -d $(MAN_PREFIX)/man1
	@# tuxpaint.1
	@cp man/en/tuxpaint.1 $(MAN_PREFIX)/man1/
	@gzip -f $(MAN_PREFIX)/man1/tuxpaint.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tuxpaint.1.gz
	@# tuxpaint-import.1
	@cp man/en/tuxpaint-import.1 $(MAN_PREFIX)/man1/
	@gzip -f $(MAN_PREFIX)/man1/tuxpaint-import.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tuxpaint-import.1.gz
	@# tp-magic-config.1
	@cp man/en/tp-magic-config.1 $(MAN_PREFIX)/man1/
	@gzip -f $(MAN_PREFIX)/man1/tp-magic-config.1
	@chmod a+rx,g-w,o-w $(MAN_PREFIX)/man1/tp-magic-config.1.gz
	@echo
	for l in `ls -d man/*.UTF-8 | cut -d '/' -f 2`; do \
		DEST=$(MAN_PREFIX)/$$l/man1 ; \
		echo "...Installing $$l man pages into $$DEST..." ; \
		install -d $$DEST ; \
		cp man/$$l/tuxpaint.1 $$DEST ; \
		gzip -f $$DEST/tuxpaint.1 ; \
		chmod a+rx,g-w,o-w $$DEST/tuxpaint.1.gz ; \
	done
	@# FIXME: The other man pages aren't localizable yet -bjk 2021.08.14

# Install the support files for macOS application bundle
.PHONY: install-macbundle
install-macbundle:
	@echo
	@echo "...Installing macOS App Bundle Support Files..."
	@install -d -m 755 $(BUNDLE)/Contents/MacOS
	@install -d -m 755 $(BUNDLE)/Contents/Resources
	@install -d -m 755 $(BUNDLE)/Contents/lib
	@install -m 755 tuxpaint $(BUNDLE)/Contents/MacOS
	@install -m 644 macos/PkgInfo $(BUNDLE)/Contents
	@VER_VERSION=$(VER_VERSION)$(VER_FLAVOR) macos/template.sh macos/Info.plist.shdoc > macos/Info.plist
	@install -m 644 macos/Info.plist $(BUNDLE)/Contents
	@install -m 644 macos/tuxpaint.icns $(BUNDLE)/Contents/Resources
	@macos/build-app.sh

# Install the support files for iOS application bundle
.PHONY: install-iosbundle
install-iosbundle:
	@echo
	@echo "...Installing iOS App Bundle Support Files..."
	@install -m 755 tuxpaint $(BUNDLE)
	@install -m 644 ios/PkgInfo $(BUNDLE)
	@VER_VERSION=$(VER_VERSION)$(VER_FLAVOR) ios/template.sh ios/Info.plist.shdoc > ios/Info.plist
	@install -m 644 ios/Info.plist $(BUNDLE)
	@install -m 644 ios/tuxpaint.icns $(BUNDLE)
	@install -m 644 ios/tuxpaint.cfg $(BUNDLE)
	@install -m 644 ios/Splash.png $(BUNDLE)
	@ibtool --compile $(BUNDLE)/Splash.storyboardc ios/Splash.storyboard


# Create DMG for macOS
TuxPaint.dmg:
	@echo
	@echo "...Creating DMG Distribution File..."
	@macos/build-dmg.sh


# Build the program!

tuxpaint:	obj/tuxpaint.o obj/i18n.o obj/im.o obj/cursor.o obj/pixels.o \
		obj/rgblinear.o obj/playsound.o obj/fonts.o obj/parse.o obj/fill.o \
		obj/progressbar.o obj/dirwalk.o obj/get_fname.o obj/onscreen_keyboard.o \
		obj/gifenc.o obj/sounds.o \
		$(ARCH_LIBS)
	@echo
	@echo "...Linking Tux Paint..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(FRIBIDI_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-o tuxpaint $^ \
		$(SDL_LIBS) $(SVG_LIB) $(ARCH_LINKS)
	@$(RAD_CMD)
	@$(RSRC_CMD)
	@$(MIMESET_CMD)


# Build the object for the program!

obj/tuxpaint.o:	src/tuxpaint.c \
		src/i18n.h src/im.h src/cursor.h src/pixels.h \
		src/rgblinear.h src/playsound.h src/fonts.h \
		src/fill.h src/fill_tools.h \
		src/progressbar.h src/dirwalk.h src/get_fname.h \
		src/compiler.h src/debug.h \
		src/tools.h src/titles.h src/colors.h src/shapes.h \
		src/sounds.h src/tip_tux.h src/great.h \
		src/tp_magic_api.h src/parse.h src/onscreen_keyboard.h \
		src/gifenc.h src/platform.h \
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
		$(ARCH_HEADERS)
	@echo
	@echo "...Compiling Tux Paint from source..."
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(FRIBIDI_CFLAGS) $(SVG_CFLAGS) $(MOUSE_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/tuxpaint.c -o obj/tuxpaint.o

# Broke gperf|sed up into two steps so that it will fail properly if gperf is not installed; there's probably a more elegant solution -bjk 2009.11.20
obj/parse.c:	obj/parse_step1.c
	@echo
	@echo "...Generating the command-line and config file parser (STEP 2)..."
	@sed -e 's/^const struct/static const struct/' -e 's/_GNU/_TUX/' obj/parse_step1.c > obj/parse.c

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
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) $(ARCH_DEFS) \
		-c obj/parse.c -o obj/parse.o

obj/i18n.o:	src/i18n.c src/i18n.h src/debug.h
	echo
	echo "...Compiling i18n support..."
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/i18n.c -o obj/i18n.o

obj/im.o:	src/im.c src/im.h src/debug.h
	@echo
	@echo "...Compiling IM support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/im.c -o obj/im.o

obj/get_fname.o:	src/get_fname.c src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling filename support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/get_fname.c -o obj/get_fname.o

obj/fonts.o:	src/fonts.c src/fonts.h src/dirwalk.h src/progressbar.h \
		src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling font support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/fonts.c -o obj/fonts.o

obj/dirwalk.o:	src/dirwalk.c src/dirwalk.h src/progressbar.h src/fonts.h \
		src/debug.h
	@echo
	@echo "...Compiling directory-walking support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/dirwalk.c -o obj/dirwalk.o

obj/cursor.o:	src/cursor.c src/cursor.h src/debug.h
	@echo
	@echo "...Compiling cursor support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(MOUSE_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/cursor.c -o obj/cursor.o

obj/pixels.o:	src/pixels.c src/pixels.h src/compiler.h src/debug.h
	@echo
	@echo "...Compiling pixel functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/pixels.c -o obj/pixels.o

obj/gifenc.o:	src/gifenc.c src/gifenc.h
	@echo
	@echo "...Compiling animated GIF export libary..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/gifenc.c -o obj/gifenc.o

obj/playsound.o:	src/playsound.c src/playsound.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling sound playback functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/playsound.c -o obj/playsound.o

obj/fill.o:	src/fill.c src/fill.h \
		src/rgblinear.h src/playsound.h src/pixels.h
	@echo
	@echo "...Compiling flood fill tool..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/fill.c -o obj/fill.o

obj/progressbar.o:	src/progressbar.c src/progressbar.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling progress bar functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/progressbar.c -o obj/progressbar.o

obj/rgblinear.o:	src/rgblinear.c src/rgblinear.h \
			src/compiler.h src/debug.h
	@echo
	@echo "...Compiling RGB to Linear functions..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/rgblinear.c -o obj/rgblinear.o

obj/sounds.o:	src/sounds.c src/sounds.h
	@echo
	@echo "...Compiling sound effect list..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/sounds.c -o obj/sounds.o


obj/BeOS_print.o:	src/BeOS_print.cpp src/BeOS_print.h
	@echo
	@echo "...Compiling BeOS print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/BeOS_print.cpp -o obj/BeOS_print.o

obj/win32_print.o:	src/win32_print.c src/win32_print.h src/debug.h
	@echo
	@echo "...Compiling win32 print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/win32_print.c -o obj/win32_print.o

obj/win32_trash.o:	src/win32_trash.c src/debug.h
	@echo
	@echo "...Compiling win32 trash support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/win32_trash.c -o obj/win32_trash.o

obj/postscript_print.o:	src/postscript_print.c \
			src/postscript_print.h src/debug.h
	@echo
	@echo "...Compiling PostScript print support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/postscript_print.c -o obj/postscript_print.o

obj/macos.o: src/macos.m src/macos.h src/platform.h src/debug.h
	@echo
	@echo "...Compiling macOS support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/macos.m -o obj/macos.o

obj/ios.o: src/ios.m src/ios.h src/platform.h src/debug.h
	@echo
	@echo "...Compiling iOS support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/ios.m -o obj/ios.o

obj/resource.o:	win32/resources.rc win32/resource.h
	@echo
	@echo "...Compiling win32 resources..."
	@$(WINDRES) -i win32/resources.rc -o obj/resource.o

obj/onscreen_keyboard.o:	src/onscreen_keyboard.c src/onscreen_keyboard.h src/dirwalk.h src/progressbar.h \
		src/get_fname.h src/debug.h
	@echo
	@echo "...Compiling on screen keyboard support..."
	@$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SDL_CFLAGS) $(DEFS) $(ARCH_DEFS) \
		-c src/onscreen_keyboard.c -o obj/onscreen_keyboard.o

src/tp_magic_api.h:	src/tp_magic_api.h.in
	@echo
	@echo "...Generating 'Magic' tool API development header file..."
	@(echo "/*\n\n\n\n\n\n\n\nDO NOT EDIT ME!\n\n\n\n\n\n\n\n*/" ; cat src/tp_magic_api.h.in) | sed -e s/__APIVERSION__/$(MAGIC_API_VERSION)/ > src/tp_magic_api.h


tp-magic-config:	src/tp-magic-config.sh.in
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

# FIXME: Expose SDL_rotozoom to Magic API? -bjk 2021.09.06
windows_MAGIC_SDL_LIBS:=-L/usr/local/lib $(LIBMINGW) $(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf $(SDL_MIXER_LIB)
macos_MAGIC_SDL_LIBS:=-L/usr/local/lib $(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf $(SDL_MIXER_LIB)
ios_MAGIC_SDL_LIBS:=$(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf $(SDL_MIXER_LIB)
beos_MAGIC_SDL_LIBS:=-L/usr/local/lib $(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf $(SDL_MIXER_LIB)
linux_MAGIC_SDL_LIBS:=-L/usr/local/lib $(shell $(PKG_CONFIG) $(SDL_PCNAME) --libs) -lSDL2_image -lSDL2_ttf $(SDL_MIXER_LIB)
MAGIC_SDL_LIBS:=$($(OS)_MAGIC_SDL_LIBS)

windows_MAGIC_ARCH_LINKS=-lintl $(PNG)
macos_MAGIC_ARCH_LINKS=-lintl $(PNG)
ios_MAGIC_ARCH_LINKS=-lintl -ljpeg $(PNG) $(shell $(PKG_CONFIG) --libs libtiff-4 libwebp libmpg123 ogg vorbisenc vorbisidec)
beos_MAGIC_ARCH_LINKS:=-lintl $(PNG)
linux_MAGIC_ARCH_LINKS:=-lintl $(PNG)
MAGIC_ARCH_LINKS:=$($(OS)_MAGIC_ARCH_LINKS)

windows_PLUGIN_LIBS:=$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS)
macos_PLUGIN_LIBS:=$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS)
ios_PLUGIN_LIBS:=$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS)
beos_PLUGIN_LIBS:="$(MAGIC_SDL_LIBS) $(MAGIC_ARCH_LINKS) $(MAGIC_SDL_CPPFLAGS)"
linux_PLUGIN_LIBS:=
PLUGIN_LIBS:=$($(OS)_PLUGIN_LIBS)

#MAGIC_CFLAGS:=-g3 -O2 -fvisibility=hidden -fno-common -W -Wstrict-prototypes -Wmissing-prototypes -Wall $(MAGIC_SDL_CPPFLAGS) -Isrc/
MAGIC_CFLAGS:=-g3 -O2 -fno-common -W -Wstrict-prototypes -Wmissing-prototypes -Wall $(MAGIC_SDL_CPPFLAGS) -Isrc/ $(ARCH_CFLAGS)
SHARED_FLAGS:=-shared -fpic -lm

MAGIC_C:=$(wildcard magic/src/*.c)
MAGIC_SO:=$(patsubst magic/src/%.c,magic/%.$(SO_TYPE),$(MAGIC_C))

$(MAGIC_SO): magic/%.$(SO_TYPE): magic/src/%.c src/tp_magic_api.h
	$(CC) $(MAGIC_CFLAGS) $(LDFLAGS) $(SHARED_FLAGS) -o $@ $< $(PLUGIN_LIBS)
# Probably should separate the various flags like the following:
#	$(CC) $(PLUG_CPPFLAGS) $(PLUG_CFLAGS) $(PLUG_LDFLAGS) -o $@ $< $(PLUG_LIBS)

.PHONY: magic-plugins
magic-plugins:	src/tp_magic_api.h $(MAGIC_SO)

test-png:	src/test-png.c
	$(CC) $(PNG_CFLAGS) src/test-png.c -o test-png $(PNG)

