# Makefile for Tux Paint docs
# 
# Uses "links" to convert docs from HTML to plain text.
# (Normally only ran by the developers after updating the HTML, prior to
# release.)
#
# Bill Kendrick
# bill@newbreedsoftware.com
# 
# Sept. 4, 2005 - December 3, 2017
# $Id$

LINKS_OPTIONS:=-dump -codepage utf8
LINKS:=links $(LINKS_OPTIONS)

# FIXME: Support finding files in ??/html/ and ??_??/html/)
COPYING_HTMLFILES:=$(wildcard COPYING/html/*.html)
COPYING_TEXTFILES:=$(patsubst COPYING/html/%.html,COPYING/%.txt,$(COPYING_HTMLFILES))

EN_HTMLFILES:=$(wildcard en/html/*.html)
EN_TEXTFILES:=$(patsubst en/html/%.html,en/%.txt,$(EN_HTMLFILES))

ES_HTMLFILES:=$(wildcard es/html/*.html)
ES_TEXTFILES:=$(patsubst es/html/%.html,es/%.txt,$(ES_HTMLFILES))

FR_HTMLFILES:=$(wildcard fr/html/*.html)
FR_TEXTFILES:=$(patsubst fr/html/%.html,fr/%.txt,$(FR_HTMLFILES))

GL_HTMLFILES:=$(wildcard gl/html/*.html)
GL_TEXTFILES:=$(patsubst gl/html/%.html,gl/%.txt,$(GL_HTMLFILES))

IT_HTMLFILES:=$(wildcard it/html/*.html)
IT_TEXTFILES:=$(patsubst it/html/%.html,it/%.txt,$(IT_HTMLFILES))

JA_HTMLFILES:=$(wildcard ja/html/*.html)
JA_TEXTFILES:=$(patsubst ja/html/%.html,ja/%.txt,$(JA_HTMLFILES))

NL_HTMLFILES:=$(wildcard nl/html/*.html)
NL_TEXTFILES:=$(patsubst nl/html/%.html,nl/%.txt,$(NL_HTMLFILES))

RU_HTMLFILES:=$(wildcard ru/html/*.html)
RU_TEXTFILES:=$(patsubst ru/html/%.html,ru/%.txt,$(RU_HTMLFILES))

ZH_CN_HTMLFILES:=$(wildcard zh_cn/html/*.html)
ZH_CN_TEXTFILES:=$(patsubst zh_cn/html/%.html,zh_cn/%.txt,$(ZH_CN_HTMLFILES))

ZH_TW_HTMLFILES:=$(wildcard zh_tw/html/*.html)
ZH_TW_TEXTFILES:=$(patsubst zh_tw/html/%.html,zh_tw/%.txt,$(ZH_TW_HTMLFILES))


.PHONY: all
all:	$(COPYING_TEXTFILES) \
	$(EN_TEXTFILES) \
	$(ES_TEXTFILES) \
	$(FR_TEXTFILES) \
	$(GL_TEXTFILES) \
	$(IT_TEXTFILES) \
	$(JA_TEXTFILES) \
	$(NL_TEXTFILES) \
	$(RU_TEXTFILES) \
	$(ZH_CN_TEXTFILES) \
	$(ZH_TW_TEXTFILES)

.PHONY: clean
clean:
	-rm \
	$(COPYING_TEXTFILES) \
	$(EN_TEXTFILES) \
	$(ES_TEXTFILES) \
	$(FR_TEXTFILES) \
	$(GL_TEXTFILES) \
	$(IT_TEXTFILES) \
	$(JA_TEXTFILES) \
	$(NL_TEXTFILES) \
	$(RU_TEXTFILES) \
	$(ZH_CN_TEXTFILES) \
	$(ZH_TW_TEXTFILES)

$(COPYING_TEXTFILES): COPYING/%.txt: COPYING/html/%.html
	$(LINKS) $< > $@

$(EN_TEXTFILES): en/%.txt: en/html/%.html
	$(LINKS) $< > $@

$(ES_TEXTFILES): es/%.txt: es/html/%.html
	$(LINKS) $< > $@

$(FR_TEXTFILES): fr/%.txt: fr/html/%.html
	$(LINKS) $< > $@

$(GL_TEXTFILES): gl/%.txt: gl/html/%.html
	$(LINKS) $< > $@

$(IT_TEXTFILES): it/%.txt: it/html/%.html
	$(LINKS) $< > $@

$(JA_TEXTFILES): ja/%.txt: ja/html/%.html
	$(LINKS) $< > $@

$(NL_TEXTFILES): nl/%.txt: nl/html/%.html
	$(LINKS) $< > $@

$(RU_TEXTFILES): ru/%.txt: ru/html/%.html
	$(LINKS) $< > $@

$(ZH_CN_TEXTFILES): zh_cn/%.txt: zh_cn/html/%.html
	$(LINKS) $< > $@

$(ZH_TW_TEXTFILES): zh_tw/%.txt: zh_tw/html/%.html
	$(LINKS) $< > $@

