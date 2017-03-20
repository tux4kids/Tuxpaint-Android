LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tp_android_assets_fopen

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := tp_android_assets_fopen.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
