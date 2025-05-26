# Makefile for Tux Paint docs
#
# Uses "w3m" to convert docs from HTML to plain text.
# (Normally only ran by the developers after updating the HTML, prior to
# release.)
#
# Bill Kendrick
# bill@newbreedsoftware.com
#
# Sept. 4, 2005 - April 19, 2025

HTML2TXT_OPTIONS:=-dump -cols 80 -no-graph -o pseudo_inlines=f -o display_image=f -T text/html
HTML2TXT:=./w3m.sh $(HTML2TXT_OPTIONS)

EN_HTMLFILES:=$(wildcard en/html/*.html)
EN_TEXTFILES:=$(patsubst en/html/%.html,en/%.txt,$(EN_HTMLFILES))

ES_HTMLFILES:=$(wildcard es_ES.UTF-8/html/*.html)
ES_TEXTFILES:=$(patsubst es_ES.UTF-8/html/%.html,es_ES.UTF-8/%.txt,$(ES_HTMLFILES))

FR_HTMLFILES:=$(wildcard fr_FR.UTF-8/html/*.html)
FR_TEXTFILES:=$(patsubst fr_FR.UTF-8/html/%.html,fr_FR.UTF-8/%.txt,$(FR_HTMLFILES))

GL_HTMLFILES:=$(wildcard gl_ES.UTF-8/html/*.html)
GL_TEXTFILES:=$(patsubst gl_ES.UTF-8/html/%.html,gl_ES.UTF-8/%.txt,$(GL_HTMLFILES))

IS_HTMLFILES:=$(wildcard is_IS.UTF-8/html/*.html)
IS_TEXTFILES:=$(patsubst is_IS.UTF-8/html/%.html,is_IS.UTF-8/%.txt,$(IS_HTMLFILES))

IT_HTMLFILES:=$(wildcard it/html/*.html)
IT_TEXTFILES:=$(patsubst it/html/%.html,it/%.txt,$(IT_HTMLFILES))

JA_HTMLFILES:=$(wildcard ja_JP.UTF-8/html/*.html)
JA_TEXTFILES:=$(patsubst ja_JP.UTF-8/html/%.html,ja_JP.UTF-8/%.txt,$(JA_HTMLFILES))

NL_HTMLFILES:=$(wildcard nl/html/*.html)
NL_TEXTFILES:=$(patsubst nl/html/%.html,nl/%.txt,$(NL_HTMLFILES))

RU_HTMLFILES:=$(wildcard ru/html/*.html)
RU_TEXTFILES:=$(patsubst ru/html/%.html,ru/%.txt,$(RU_HTMLFILES))

SV_HTMLFILES:=$(wildcard sv_SE.UTF-8/html/*.html)
SV_TEXTFILES:=$(patsubst sv_SE.UTF-8/html/%.html,sv_SE.UTF-8/%.txt,$(SV_HTMLFILES))

SQ_HTMLFILES:=$(wildcard sq_AL.UTF-8/html/*.html)
SQ_TEXTFILES:=$(patsubst sq_AL.UTF-8/html/%.html,sq_AL.UTF-8/%.txt,$(SQ_HTMLFILES))

ZH_CN_HTMLFILES:=$(wildcard zh_cn/html/*.html)
ZH_CN_TEXTFILES:=$(patsubst zh_cn/html/%.html,zh_cn/%.txt,$(ZH_CN_HTMLFILES))

ZH_TW_HTMLFILES:=$(wildcard zh_tw/html/*.html)
ZH_TW_TEXTFILES:=$(patsubst zh_tw/html/%.html,zh_tw/%.txt,$(ZH_TW_HTMLFILES))


.PHONY: all
all:	$(EN_TEXTFILES) \
	$(ES_TEXTFILES) \
	$(FR_TEXTFILES) \
	$(GL_TEXTFILES) \
	$(IS_TEXTFILES) \
	$(IT_TEXTFILES) \
	$(JA_TEXTFILES) \
	$(NL_TEXTFILES) \
	$(RU_TEXTFILES) \
	$(SQ_TEXTFILES) \
	$(SV_TEXTFILES) \
	$(ZH_CN_TEXTFILES) \
	$(ZH_TW_TEXTFILES)

.PHONY: clean
clean:
	-rm \
	$(EN_TEXTFILES) \
	$(ES_TEXTFILES) \
	$(FR_TEXTFILES) \
	$(GL_TEXTFILES) \
	$(IS_TEXTFILES) \
	$(IT_TEXTFILES) \
	$(JA_TEXTFILES) \
	$(NL_TEXTFILES) \
	$(RU_TEXTFILES) \
	$(SQ_TEXTFILES) \
	$(SV_TEXTFILES) \
	$(ZH_CN_TEXTFILES) \
	$(ZH_TW_TEXTFILES)

$(EN_TEXTFILES): en/%.txt: en/html/%.html
	$(HTML2TXT) $< > $@

$(ES_TEXTFILES): es_ES.UTF-8/%.txt: es_ES.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(FR_TEXTFILES): fr_FR.UTF-8/%.txt: fr_FR.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(GL_TEXTFILES): gl_ES.UTF-8/%.txt: gl_ES.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(IS_TEXTFILES): is_IS.UTF-8/%.txt: is_IS.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(IT_TEXTFILES): it/%.txt: it/html/%.html
	$(HTML2TXT) $< > $@

$(JA_TEXTFILES): ja_JP.UTF-8/%.txt: ja_JP.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(NL_TEXTFILES): nl/%.txt: nl/html/%.html
	$(HTML2TXT) $< > $@

$(RU_TEXTFILES): ru/%.txt: ru/html/%.html
	$(HTML2TXT) $< > $@

$(SQ_TEXTFILES): sq_AL.UTF-8/%.txt: sq_AL.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(SV_TEXTFILES): sv_SE.UTF-8/%.txt: sv_SE.UTF-8/html/%.html
	$(HTML2TXT) $< > $@

$(ZH_CN_TEXTFILES): zh_cn/%.txt: zh_cn/html/%.html
	$(HTML2TXT) $< > $@

$(ZH_TW_TEXTFILES): zh_tw/%.txt: zh_tw/html/%.html
	$(HTML2TXT) $< > $@
