LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SDL2_Pango

LOCAL_SRC_FILES := \
	src/SDL2_Pango.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src

LOCAL_SHARED_LIBRARIES :=		\
	tuxpaint_glib			\
	tuxpaint_pango			\
	tuxpaint_fontconfig		\
	SDL2

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
