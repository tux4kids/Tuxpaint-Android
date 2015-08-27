LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_pango

# Although SYSCONFDIR and LIBDIR are configured the place where TuxPaint lives,
# there is nothing in fact.
LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H \
	-DG_LOG_DOMAIN=\"Pango\" \
	-DPANGO_ENABLE_BACKEND \
	-DPANGO_ENABLE_ENGINE \
	-DSYSCONFDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/pango\" \
	-DLIBDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/pango\" \
	$(NULL)

LOCAL_C_INCLUDES :=         \
	$(LOCAL_PATH)/pango \
	$(LOCAL_PATH)/pango/mini-fribidi \
	$(LOCAL_PATH)			 \
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

# source files for libpango, libpangoft2, libpangocairo
SRC_FILES :=  \
	break.c					\
	ellipsize.c				\
	fonts.c					\
	glyphstring.c				\
	modules.c				\
	pango-attributes.c			\
	pango-bidi-type.c			\
	pango-color.c				\
	pango-context.c				\
	pango-coverage.c			\
	pango-engine.c				\
	pango-fontmap.c				\
	pango-fontset.c				\
	pango-glyph-item.c			\
	pango-gravity.c				\
	pango-item.c				\
	pango-language.c			\
	pango-layout.c				\
	pango-markup.c				\
	pango-matrix.c				\
	pango-renderer.c			\
	pango-script.c				\
	pango-tabs.c				\
	pango-utils.c				\
	reorder-items.c				\
	shape.c					\
	pango-enum-types.c			\
	pangofc-font.c	   	\
	pangofc-fontmap.c	\
	pangofc-decoder.c	\
	pangofc-shape.c		\
	pangoft2.c		\
	pangoft2-fontmap.c	\
	pangoft2-render.c	\
	pango-ot-buffer.c	\
	pango-ot-info.c		\
	pango-ot-ruleset.c      \
	pango-ot-tag.c		\
	pangocairo-context.c    	\
	pangocairo-font.c		\
	pangocairo-fontmap.c    	\
	pangocairo-render.c		\
	pangocairo-fcfont.c		\
	pangocairo-fcfontmap.c		\
	$(NULL)

LOCAL_SRC_FILES := $(addprefix pango/, $(SRC_FILES))

LOCAL_SRC_FILES += 	\
	pango/mini-fribidi/fribidi.c	\
	pango/mini-fribidi/fribidi_char_type.c 	\
	pango/mini-fribidi/fribidi_types.c		\
	$(NULL)

LOCAL_SHARED_LIBRARIES := \
	tuxpaint_glib	 \
	tuxpaint_cairo \
	tuxpaint_freetype \
	tuxpaint_fontconfig \
	tuxpaint_harfbuzz_ng \
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
