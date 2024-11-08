LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_fribidi

LOCAL_SRC_FILES := \
	lib/fribidi.c \
	lib/fribidi-arabic.c \
	lib/fribidi-bidi.c \
	lib/fribidi-bidi-types.c \
	lib/fribidi-brackets.c \
	lib/fribidi-deprecated.c \
	lib/fribidi-joining.c \
	lib/fribidi-joining-types.c \
	lib/fribidi-mirroring.c \
	lib/fribidi-run.c \
	lib/fribidi-shape.c \
	lib/fribidi-char-sets.c \
	lib/fribidi-char-sets-iso8859-6.c \
	lib/fribidi-char-sets-cap-rtl.c \
	lib/fribidi-char-sets-iso8859-8.c \
	lib/fribidi-char-sets-cp1255.c \
	lib/fribidi-char-sets-cp1256.c \
	lib/fribidi-char-sets-utf8.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/lib \
	$(LOCAL_PATH)/charset \
	$(LOCAL_PATH)

LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)
