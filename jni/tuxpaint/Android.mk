LOCAL_PATH := $(call my-dir)

########### Main tuxpaint library ###########
include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../SDL2/src/main/android \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/mouse \
	$(NULL)

LOCAL_SRC_FILES := \
	../SDL2/src/main/android/SDL_android_main.c \
	src/tuxpaint.c \
	src/i18n.c \
	src/im.c \
	src/get_fname.c \
	src/fonts.c \
	src/dirwalk.c \
	src/parse.c \
	src/cursor.c \
	src/pixels.c \
	src/playsound.c \
	src/progressbar.c \
	src/rgblinear.c \
	src/onscreen_keyboard.c \
	src/android_print.c \
	src/android_mbstowcs.c \
	src/android_assets.c

MY_CFLAGS:= -O0 -g -W -Wall -fno-common -ffloat-store \
	-Wcast-align -Wredundant-decls \
	-Wbad-function-cast -Wwrite-strings \
	-Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wstrict-aliasing=2 \
	-include $(LOCAL_PATH)"/../tp-assets-fopen/tp_android_assets_fopen.h"

MY_VER_VERSION :=0.9.23
MY_VER_DATE :=$(shell date +"%Y-%m-%d")
MY_NOSOUNDFLAG := 
# MY_NOSOUNDFLAG := -DNOSOUND
MY_NOPANGOFLAG := 
# MY_NOPANGOFLAG :=  -DNO_SDLPANGO
MY_NOSVGFLAG := 
# MY_NOSVGFLAG := -DNOSVG
MY_INTERNAL_DIR := "/data/data/org.tuxpaint/"
MY_ASSETS_DIR := ""
# Data:
MY_DATA_PREFIX := $(MY_ASSETS_DIR)data/
# Doc files, but DOC_PREFIX is useless on the Android currently 
MY_DOC_PREFIX := $(MY_ASSETS_DIR)doc/
# Locale files
MY_LOCALE_PREFIX := $(MY_ASSETS_DIR)locale
# IM files
MY_IM_PREFIX := $(MY_DATA_PREFIX)im/
# 'System-wide' Config file, but CONFDIR is useless on the Android currently
MY_CONFDIR := $(MY_ASSETS_DIR)etc/
# Magic Tool plug-ins
MY_MAGIC_PREFIX := $(MY_INTERNAL_DIR)lib/

MY_DEFS := \
	-DVER_DATE=\"$(MY_VER_DATE)\" \
	-DVER_VERSION=\"$(MY_VER_VERSION)\" \
	-DDATA_PREFIX=\"$(MY_DATA_PREFIX)\" \
	-DDOC_PREFIX=\"$(MY_DOC_PREFIX)\" \
	-DLOCALEDIR=\"$(MY_LOCALE_PREFIX)\" \
	-DIMDIR=\"$(MY_IM_PREFIX)\" \
	-DCONFDIR=\"$(MY_CONFDIR)\" \
	-DMAGIC_PREFIX=\"$(MY_MAGIC_PREFIX)\" \
	-DHAVE_STRCASESTR \
	$(MY_NOSOUNDFLAG) $(MY_NOSVGFLAG) $(MY_NOPANGOFLAG)

LOCAL_CFLAGS := \
	$(MY_CFLAGS) \
	$(MY_DEFS)

LOCAL_LDLIBS := \
	-lz -llog -lGLESv1_CM -lGLESv2 -landroid \
	$(NULL)

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf SDL2_Pango tuxpaint_intl tuxpaint_fribidi tuxpaint_png tuxpaint_rsvg tuxpaint_cairo tp_android_assets_fopen

include $(BUILD_SHARED_LIBRARY)

########### Magic plugin libraries ###########
MAGIC_FILES := $(wildcard $(LOCAL_PATH)/magic/src/*.c)
MAGIC_NAMES := $(patsubst %.c, %, $(notdir $(MAGIC_FILES)))
$(foreach _magic, $(MAGIC_NAMES),\
    $(eval include $(CLEAR_VARS))\
    $(eval LOCAL_MODULE := $(_magic))\
    $(eval LOCAL_C_INCLUDES := $(LOCAL_PATH)/src)\
    $(eval MAGIC_CFLAGS:=-g3 -O2 -fno-common -W -Wstrict-prototypes -Wmissing-prototypes -Wall)\
    $(eval LOCAL_SRC_FILES := magic/src/$(_magic).c)\
    $(eval LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf tuxpaint_intl)\
    $(eval include $(BUILD_SHARED_LIBRARY))\
)
