#Makefile.am for SDL_Pango library/DLL

lib_LTLIBRARIES = libSDL2_Pango.la

libSDL2_Pangoincludedir = $(includedir)
libSDL2_Pangoinclude_HEADERS = SDL2_Pango.h

libSDL2_Pango_la_SOURCES = SDL2_Pango.c

DEFS = @DEFS@ -DBUILD_DLL
libSDL2_Pango_la_LDFLAGS = \
	-no-undefined		\
    -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

DISTCLEANFILES = *~ *~c *~h *.cross.cache inc
