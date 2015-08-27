# NMake Makefile to build Introspection Files for GDK-Pixbuf

!include detectenv-msvc.mak

APIVERSION = 2.0

CHECK_PACKAGE = gio-2.0

!include introspection-msvc.mak

!if "$(BUILD_INTROSPECTION)" == "TRUE"
all: setgirbuildnev GdkPixbuf-$(APIVERSION).gir GdkPixbuf-$(APIVERSION).typelib

gdkpixbuf_list:
	@-echo Generating Filelist to Introspect for GDK-Pixbuf...
	$(PYTHON2) gen-file-list-gdkpixbuf.py

setgirbuildnev:
	@-set CC=$(CC)
	@-set PYTHONPATH=$(BASEDIR)\lib\gobject-introspection
	@-set PATH=win32\vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(BASEDIR)\bin;$(PATH);$(MINGWDIR)\bin
	@-set PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)
	@-set LIB=win32\vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(LIB)

GdkPixbuf-$(APIVERSION).gir: gdkpixbuf_list
	@-echo Generating GdkPixbuf-$(APIVERSION).gir...
	$(PYTHON2) $(G_IR_SCANNER) --verbose -I.. -I..\gdk-pixbuf	\
	-I$(BASEDIR)\include\glib-2.0 -I$(BASEDIR)\lib\glib-2.0\include	\
	--namespace=GdkPixbuf --nsversion=$(APIVERSION)	\
	--include=GModule-2.0 --include=Gio-2.0	\
	--no-libtool --library=gdk_pixbuf-2.0	\
	--reparse-validate --add-include-path=$(BASEDIR)\share\gir-1.0	\
	--pkg-export gdk-pixbuf-2.0 --warn-all --strip-prefix=Gdk	\
	--c-include="gdk-pixbuf/gdk-pixbuf.h" -DGDK_PIXBUF_COMPILATION	\
	--filelist=gdkpixbuf_list -o $@

GdkPixbuf-$(APIVERSION).typelib: GdkPixbuf-$(APIVERSION).gir
	@-echo Compiling GdkPixbuf-$(APIVERSION).typelib...
	$(G_IR_COMPILER) --includedir=. --debug --verbose GdkPixbuf-$(APIVERSION).gir -o $@

install-introspection: setgirbuildnev GdkPixbuf-$(APIVERSION).gir GdkPixbuf-$(APIVERSION).typelib
	@-copy GdkPixbuf-$(APIVERSION).gir $(G_IR_INCLUDEDIR)
	@-copy /b GdkPixbuf-$(APIVERSION).typelib $(G_IR_TYPELIBDIR)

!else
all:
	@-echo $(ERROR_MSG)

install-introspection: all
!endif

clean:
	@-del /f/q GdkPixbuf-$(APIVERSION).typelib
	@-del /f/q GdkPixbuf-$(APIVERSION).gir
	@-del /f/q gdkpixbuf_list
	@-del /f/q *.pyc
