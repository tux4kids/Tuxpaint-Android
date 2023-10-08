LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)  
LOCAL_MODULE := tuxpaint_iconv  
LOCAL_CFLAGS := \
  -Wno-multichar \
  -DLIBDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/iconv\" \
  -DBUILDING_LIBICONV \
  -DIN_LIBRARY
  
LOCAL_SRC_FILES := \
    libcharset/lib/localcharset.c \
    lib/iconv.c \
    lib/relocatable.c  
  
LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/include \
  $(LOCAL_PATH)/libcharset \
  $(LOCAL_PATH)/lib \
  $(LOCAL_PATH)/libcharset/include \
  $(LOCAL_PATH)/srclib	\
  $(LOCAL_PATH)
    
LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)

