LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_intl

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/internal
	
LOCAL_SRC_FILES := internal/libintl.cpp


LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
