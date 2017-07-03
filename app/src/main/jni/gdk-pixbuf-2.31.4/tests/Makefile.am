include $(top_srcdir)/glib-tap.mk

AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gdk-pixbuf	\
	$(GDK_PIXBUF_DEBUG_FLAGS)	\
	$(GDK_PIXBUF_DEP_CFLAGS)	\
	$(NULL)

DEPS = \
	$(top_builddir)/gdk-pixbuf/libgdk_pixbuf-$(GDK_PIXBUF_API_VERSION).la

LDADD = \
	$(top_builddir)/gdk-pixbuf/libgdk_pixbuf-$(GDK_PIXBUF_API_VERSION).la 	\
	$(GDK_PIXBUF_DEP_LIBS) \
	$(GLIB_LIBS) \
	$(NULL)

noinst_PROGRAMS += 			\
	pixbuf-read			\
	pixbuf-lowmem			\
	pixbuf-random			\
	$(NULL)

test_programs = 			\
	animation			\
	pixbuf-icon-serialize 		\
	pixbuf-randomly-modified	\
	pixbuf-threads			\
	pixbuf-icc			\
	pixbuf-jpeg			\
	pixbuf-dpi			\
	pixbuf-stream			\
	pixbuf-resource			\
	pixbuf-scale			\
	pixbuf-short-gif-write		\
	pixbuf-save			\
	pixbuf-readonly-to-mutable	\
	pixbuf-composite		\
	$(NULL)

dist_installed_test_data =		\
	test-image.png			\
	test-animation.gif		\
	test-animation.ani		\
	icc-profile.jpeg		\
	icc-profile.png			\
	dpi.jpeg			\
	dpi.png				\
	dpi.tiff			\
	premature-end.png		\
	premature-end.jpg		\
	bug725582-testrotate.jpg	\
	bug725582-testrotate.png	\
	$(wildcard $(srcdir)/test-images/*)

pixbuf_icc_SOURCES = 			\
	pixbuf-icc.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_dpi_SOURCES = 			\
	pixbuf-dpi.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_scale_SOURCES = 			\
	pixbuf-scale.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_stream_SOURCES =			\
	pixbuf-stream.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_threads_SOURCES =		\
	pixbuf-threads.c		\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_icon_serialize_SOURCES =		\
	pixbuf-icon-serialize.c		\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_save_SOURCES =			\
	pixbuf-save.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_jpeg_SOURCES =			\
	pixbuf-jpeg.c			\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_short_git_write_SOURCES =	\
	pixbuf-short-gif-write.c

pixbuf_readonly_to_mutable_SOURCES =	\
	pixbuf-readonly-to-mutable.c	\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_composite_SOURCES =		\
	pixbuf-composite.c		\
	test-common.c			\
	test-common.h			\
	$(NULL)

pixbuf_resource_SOURCES = 		\
	pixbuf-resource.c		\
	test-common.c			\
	test-common.h			\
	resources.h			\
	resources.c			\
	$(NULL)

BUILT_SOURCES += resources.h resources.c
DISTCLEANFILES += 			\
	resources.h resources.c 	\
	pixbuf-save-options		\
	pixbuf-randomly-modified-image	\
	pixbuf-save-roundtrip		\
	$(NULL)
	
EXTRA_DIST += resources.gresource.xml
TESTS_ENVIRONMENT += GDK_PIXBUF_MODULE_FILE=$(top_builddir)/gdk-pixbuf/loaders.cache

resources.h: resources.gresource.xml
	$(AM_V_GEN) GDK_PIXBUF_MODULE_FILE=$(top_builddir)/gdk-pixbuf/loaders.cache GDK_PIXBUF_PIXDATA=$(top_builddir)/gdk-pixbuf/gdk-pixbuf-pixdata $(GLIB_COMPILE_RESOURCES) $(srcdir)/resources.gresource.xml --target=$@ --sourcedir=$(srcdir) --generate-header

resources.c: resources.gresource.xml
	$(AM_V_GEN) GDK_PIXBUF_MODULE_FILE=$(top_builddir)/gdk-pixbuf/loaders.cache GDK_PIXBUF_PIXDATA=$(top_builddir)/gdk-pixbuf/gdk-pixbuf-pixdata $(GLIB_COMPILE_RESOURCES) $(srcdir)/resources.gresource.xml --target=$@ --sourcedir=$(srcdir) --generate-source

-include $(top_srcdir)/git.mk
