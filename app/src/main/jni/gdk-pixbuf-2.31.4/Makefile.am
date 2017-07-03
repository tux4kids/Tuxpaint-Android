SUBDIRS = gdk-pixbuf po docs tests contrib build

EXTRA_DIST = 			\
	config.h.win32 		\
	glib-tap.mk		\
	tap-driver.sh		\
	tap-test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdk-pixbuf-2.0.pc

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCLEANFILES = 		\
	gdk-pixbuf-2.0.pc

DISTCHECK_CONFIGURE_FLAGS = 	\
	--enable-gtk-doc	\
	--enable-man		\
	--enable-introspection

MAINTAINERCLEANFILES = \
	ABOUT-NLS \
	aclocal.m4 \
	config.guess \
	config.h.in \
	config.rpath \
	config.sub \
	depcomp \
	gtk-doc.make \
	install-sh \
	ltmain.sh \
	missing

if WITH_COVERAGE
.PHONY: coverage clear-coverage

coverage:
	$(AM_V_GEN) $(LCOV) --quiet --directory $(top_builddir) --capture --output-file $(top_builddir)/coverage.info --no-checksum --compat-libtool; \
	  $(GENHTML) --quiet --output-directory $(top_builddir)/coverage $(top_builddir)/coverage.info
	@echo "file://$(abs_top_builddir)/coverage/index.html"

clear-coverage:
	$(AM_V_GEN) $(LCOV) --directory . --zerocounters
endif

-include $(top_srcdir)/git.mk
