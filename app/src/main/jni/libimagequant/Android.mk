########## File adapted from Android.mk in ../jni/tuxpaint/ ##########
##########              License GNU/GPL v2+                 ##########
LOCAL_PATH := $(call my-dir)

########### Main tuxpaint library ###########
include $(CLEAR_VARS)

LOCAL_MODULE := libimagequant

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)

LOCAL_SRC_FILES := \
	blur.c \
	kmeans.c \
	libimagequant.c \
	mediancut.c \
	mempool.c \
	nearest.c \
	pam.c

MY_CFLAGS:= -O0 -g -W -Wall -fno-common -ffloat-store \
	-Wcast-align -Wredundant-decls \
	-Wbad-function-cast -Wwrite-strings \
	-Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wstrict-aliasing=2

LOCAL_CFLAGS := \
	$(MY_CFLAGS) \
	$(MY_DEFS)

include $(BUILD_SHARED_LIBRARY)
