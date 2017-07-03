# This Android.mk is based on following files, however cannot confirm  whether right or not  
# (1) Makefile.am of libffi, 
# (2) Android.mk and Libffi.mk of platform_external_libffi https://github.com/android/platform_external_libffi

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_ffi

ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_SRC_FILES := src/arm/sysv.S src/arm/ffi.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/arm
LOCAL_CFLAGS := -DARM
endif
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_SRC_FILES := src/arm/sysv.S src/arm/ffi.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/arm
LOCAL_CFLAGS := -DARM
endif
# !!! I am not sure whether arm64-v8a is arm or aarch64
# !!! Please decide by yourself
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_SRC_FILES := src/aarch64/sysv.S src/aarch64/ffi.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/aarch64
LOCAL_CFLAGS := -DAARCH64
endif
ifeq ($(TARGET_ARCH_ABI), mips)
LOCAL_SRC_FILES += src/mips/ffi.c src/mips/o32.S src/mips/n32.S
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/mips
LOCAL_CFLAGS := -DMIPS
endif
ifeq ($(TARGET_ARCH_ABI), mips64)
LOCAL_SRC_FILES += src/mips/ffi.c src/mips/o32.S src/mips/n32.S
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/mips
LOCAL_CFLAGS := -DMIPS
endif
ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_SRC_FILES :=  src/x86/ffi.c src/x86/sysv.S src/x86/win32.S
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/x86
LOCAL_CFLAGS := -DX86
endif
ifeq ($(TARGET_ARCH_ABI), x86-64)
LOCAL_SRC_FILES := src/x86/ffi64.c src/x86/unix64.S src/x86/ffi.c src/x86/sysv.S
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/x86
LOCAL_CFLAGS := -DX86_64
endif
ifeq ($(LOCAL_SRC_FILES),)
$(info The os/architecture $(TARGET_ARCH_ABI) is not supported by libffi.)
LOCAL_SRC_FILES := your-architecture-not-supported-by-ffi-makefile.c
LOCAL_C_INCLUDES := 
LOCAL_CFLAGS := 
endif

LOCAL_SRC_FILES += \
	src/prep_cif.c 	\
	src/types.c 	\
	src/raw_api.c	\
 	src/java_raw_api.c \
	src/closures.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
