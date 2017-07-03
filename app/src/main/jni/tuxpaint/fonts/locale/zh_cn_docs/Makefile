# Makefile for tuxpaint-ttf-chinese-simplified

# Tux Paint - A simple drawing program for children.
# This is a Chinese (Simplified) TrueType Font (TTF) for use by Tux Paint

# Packaged by Bill Kendrick
# bill@newbreedsoftware.com
# http://www.newbreedsoftware.com/tuxpaint/

# January 4, 2003 - June 5, 2004


FONT=zh_cn.ttf
TP_LANG=simplified-chinese


# Where to install things:

PREFIX=/usr/local


# Data:

DATA_PREFIX=$(PREFIX)/share/tuxpaint/


# "make" with no arguments provides info on installing this tuxpaint font:

all:
	@echo
	@echo "To install the '$(TP_LANG)' Tux Paint font ($(FONT)),"
	@echo "run 'make install' (probably as 'root' superuser)"
	@echo
	@echo "The font will be placed in:"
	@echo "  $(DATA_PREFIX)fonts/locale/"
	@echo
	@if [ -x "`which tuxpaint`" ]; then echo ""; else \
	echo "(Tux Paint doesn't appear to be installed, though!!!)"; \
	echo; fi


clean:
	@echo
	@echo "There's no source here, so 'make clean' doesn't make sense!"
	@echo




# "make install" installs the font
# (depending on the *PREFIX variables at the top, you probably need
# to do this as superuser ("root"))

install:	install-font
	@echo
	@echo "All done! Now (preferably NOT as 'root' superuser),"
	@echo "you can type the command 'tuxpaint --lang $(TP_LANG)' to run the"
	@echo "program and in that language!!!"
	@echo
	@echo "For more information, see the 'tuxpaint' man page,"
	@echo "run 'tuxpaint --usage' or see $(DOC_PREFIX)README.txt"
	@echo
	@if [ -x "`which tuxpaint`" ]; then echo "Enjoy!"; else \
	echo "(Tux Paint doesn't appear to be installed, though!!!)"; fi
	@echo


# "make uninstall" should remove the various parts from their
# installation locations.  BE SURE the *PREFIX variables at the top
# are the same as they were when you installed, of course!!!

uninstall:
	@echo
	@echo "Sorry - uninstall is not implemented yet. :^("
	@echo "Uninstalling Tux Paint should uninstall all of the fonts, though"
	@echo


# Install the font:

install-font:
	@echo
	@echo "...Installing font file..."
	install -d $(DATA_PREFIX)fonts/locale
	cp -R $(FONT) $(DATA_PREFIX)fonts/locale
	chmod -R a+rX,g-w,o-w $(DATA_PREFIX)fonts/locale/

