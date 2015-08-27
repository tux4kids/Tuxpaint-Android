# Makefile to build and install the SDL_mixer library

top_builddir = .
srcdir  = .
objects = build
prefix = /usr/local
exec_prefix = ${prefix}
bindir	= $(DESTDIR)${exec_prefix}/bin
libdir  = $(DESTDIR)${exec_prefix}/lib
includedir = $(DESTDIR)${prefix}/include
datarootdir = $(DESTDIR)${prefix}/share
datadir	= ${datarootdir}
mandir	= ${datarootdir}/man
auxdir	= ./build-scripts
distpath = $(srcdir)/..
distdir = SDL2_mixer-2.0.0
distfile = $(distdir).tar.gz


EXE	= 
SHELL	= /bin/bash
CC      = gcc
CXX     = g++
CFLAGS  = -g -O2 
EXTRA_CFLAGS =  -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/local/include/SDL2   -DHAVE_SIGNAL_H -DHAVE_SETBUF -fvisibility=hidden -DHAVE_FORK -DCMD_MUSIC -DWAV_MUSIC -DMID_MUSIC -DUSE_TIMIDITY_MIDI -I$(srcdir)/timidity
LDFLAGS = 
EXTRA_LDFLAGS =  -Wl,-rpath,/usr/local/lib -L/usr/local/lib -lSDL2 -lpthread   -lm
LIBTOOL = $(SHELL) $(top_builddir)/libtool
INSTALL = /usr/bin/install -c
AR	= ar
RANLIB	= ranlib
WINDRES	= 
SDL_CFLAGS = -D_REENTRANT -I/usr/local/include/SDL2  
SDL_LIBS = -Wl,-rpath,/usr/local/lib -L/usr/local/lib -lSDL2 -lpthread  

TARGET  = libSDL2_mixer.la
OBJECTS = $(objects)/effect_position.lo $(objects)/effect_stereoreverse.lo $(objects)/effects_internal.lo $(objects)/load_aiff.lo $(objects)/load_voc.lo $(objects)/mixer.lo $(objects)/music.lo $(objects)/music_cmd.lo $(objects)/wavestream.lo $(objects)/common.lo $(objects)/ctrlmode.lo $(objects)/filter.lo $(objects)/instrum.lo $(objects)/mix.lo $(objects)/output.lo $(objects)/playmidi.lo $(objects)/readmidi.lo $(objects)/resample.lo $(objects)/sdl_a.lo $(objects)/sdl_c.lo $(objects)/tables.lo $(objects)/timidity.lo 
VERSION_OBJECTS = 
PLAYWAVE_OBJECTS = $(objects)/playwave.lo
PLAYMUS_OBJECTS = $(objects)/playmus.lo

DIST = *.txt Android.mk Makefile.in SDL2_mixer.pc.in SDL_mixer.h SDL2_mixer.spec SDL2_mixer.spec.in debian VisualC Xcode Xcode-iOS acinclude autogen.sh build-scripts configure configure.in dynamic_flac.c dynamic_flac.h dynamic_fluidsynth.c dynamic_fluidsynth.h dynamic_modplug.c dynamic_modplug.h dynamic_mod.c dynamic_mod.h dynamic_mp3.c dynamic_mp3.h dynamic_ogg.c dynamic_ogg.h effect_position.c effect_stereoreverse.c effects_internal.c effects_internal.h fluidsynth.c fluidsynth.h external gcc-fat.sh libmikmod-3.1.12.zip load_aiff.c load_aiff.h load_flac.c load_flac.h load_ogg.c load_ogg.h load_voc.c load_voc.h mixer.c music.c music_cmd.c music_cmd.h music_flac.c music_flac.h music_mad.c music_mad.h music_mod.c music_mod.h music_modplug.c music_modplug.h music_ogg.c music_ogg.h native_midi playmus.c playwave.c timidity wavestream.c wavestream.h version.rc

LT_AGE      = 0
LT_CURRENT  = 0
LT_RELEASE  = 2.0
LT_REVISION = 0
LT_LDFLAGS  = -no-undefined -rpath $(libdir) -release $(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

all: $(srcdir)/configure Makefile $(objects) $(objects)/$(TARGET) $(objects)/playwave$(EXE) $(objects)/playmus$(EXE)

$(srcdir)/configure: $(srcdir)/configure.in
	@echo "Warning, configure.in is out of date"
	#(cd $(srcdir) && sh autogen.sh && sh configure)
	@sleep 3

Makefile: $(srcdir)/Makefile.in
	$(SHELL) config.status $@

$(objects):
	$(SHELL) $(auxdir)/mkinstalldirs $@

.PHONY: all install install-hdrs install-lib install-bin uninstall uninstall-hdrs uninstall-lib uninstall-bin clean distclean dist

$(objects)/$(TARGET): $(OBJECTS) $(VERSION_OBJECTS)
	$(LIBTOOL) --mode=link $(CC) -o $@ $(OBJECTS) $(VERSION_OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) $(LT_LDFLAGS)

$(objects)/playwave$(EXE): $(objects)/playwave.lo $(objects)/$(TARGET)
	$(LIBTOOL) --mode=link $(CC) -o $@ $(objects)/playwave.lo $(SDL_CFLAGS) $(SDL_LIBS) $(LDFLAGS) $(objects)/$(TARGET)

$(objects)/playmus$(EXE): $(objects)/playmus.lo $(objects)/$(TARGET)
	$(LIBTOOL) --mode=link $(CC) -o $@ $(objects)/playmus.lo $(SDL_CFLAGS) $(SDL_LIBS) $(LDFLAGS) $(objects)/$(TARGET)

install: all install-hdrs install-lib #install-bin
install-hdrs:
	$(SHELL) $(auxdir)/mkinstalldirs $(includedir)/SDL2
	for src in $(srcdir)/SDL_mixer.h; do \
	    file=`echo $$src | sed -e 's|^.*/||'`; \
	    $(INSTALL) -m 644 $$src $(includedir)/SDL2/$$file; \
	done
	$(SHELL) $(auxdir)/mkinstalldirs $(libdir)/pkgconfig
	$(INSTALL) -m 644 SDL2_mixer.pc $(libdir)/pkgconfig/
install-lib: $(objects) $(objects)/$(TARGET)
	$(SHELL) $(auxdir)/mkinstalldirs $(libdir)
	$(LIBTOOL) --mode=install $(INSTALL) $(objects)/$(TARGET) $(libdir)/$(TARGET)
install-bin:
	$(SHELL) $(auxdir)/mkinstalldirs $(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 $(objects)/playwave$(EXE) $(bindir)/playwave$(EXE)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 $(objects)/playmus$(EXE) $(bindir)/playmus$(EXE)

uninstall: uninstall-hdrs uninstall-lib uninstall-bin
uninstall-hdrs:
	for src in $(srcdir)/SDL_mixer.h; do \
	    file=`echo $$src | sed -e 's|^.*/||'`; \
	    rm -f $(includedir)/SDL2/$$file; \
	done
	-rmdir $(includedir)/SDL2
	rm -f $(libdir)/pkgconfig/SDL2_mixer.pc
	-rmdir $(libdir)/pkgconfig
uninstall-lib:
	$(LIBTOOL) --mode=uninstall rm -f $(libdir)/$(TARGET)
uninstall-bin:
	rm -f $(bindir)/playwave$(EXE)
	rm -f $(bindir)/playmus$(EXE)

clean:
	rm -rf $(objects)

distclean: clean
	rm -f Makefile
	rm -f config.status config.cache config.log libtool
	rm -f SDL2_mixer.pc
	rm -rf $(srcdir)/autom4te*
	find $(srcdir) \( \
	    -name '*~' -o \
	    -name '*.bak' -o \
	    -name '*.old' -o \
	    -name '*.rej' -o \
	    -name '*.orig' -o \
	    -name '.#*' \) \
	    -exec rm -f {} \;

dist $(distfile):
	$(SHELL) $(auxdir)/mkinstalldirs $(distdir)
	(cd $(srcdir); tar cf - $(DIST)) | (cd $(distdir); tar xf -)
	rm -rf `find $(distdir) -name .svn`
	rm -f `find $(distdir) -name '.#*'`
	tar cvf - $(distdir) | gzip --best >$(distfile)
	rm -rf $(distdir)

rpm: $(distfile)
	rpmbuild -ta $?

# Build rules for objects
-include $(OBJECTS:.lo=.d)

$(objects)/effect_position.lo: ./effect_position.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/effect_stereoreverse.lo: ./effect_stereoreverse.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/effects_internal.lo: ./effects_internal.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/load_aiff.lo: ./load_aiff.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/load_voc.lo: ./load_voc.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/mixer.lo: ./mixer.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/music.lo: ./music.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/music_cmd.lo: ./music_cmd.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/wavestream.lo: ./wavestream.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/common.lo: ./timidity/common.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/ctrlmode.lo: ./timidity/ctrlmode.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/filter.lo: ./timidity/filter.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/instrum.lo: ./timidity/instrum.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/mix.lo: ./timidity/mix.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/output.lo: ./timidity/output.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/playmidi.lo: ./timidity/playmidi.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/readmidi.lo: ./timidity/readmidi.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/resample.lo: ./timidity/resample.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/sdl_a.lo: ./timidity/sdl_a.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/sdl_c.lo: ./timidity/sdl_c.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/tables.lo: ./timidity/tables.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 
$(objects)/timidity.lo: ./timidity/timidity.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@ 


-include $(PLAYWAVE_OBJECTS:.lo=.d)

$(objects)/playwave.lo: ./playwave.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@

-include $(PLAYMUS_OBJECTS:.lo=.d)

$(objects)/playmus.lo: ./playmus.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(EXTRA_CFLAGS) -MMD -MT $@ -c $< -o $@
