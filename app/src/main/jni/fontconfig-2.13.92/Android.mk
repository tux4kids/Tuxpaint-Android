LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := tuxpaint_fontconfig

 fontconfig_flags := \
 -DFONTCONFIG_PATH=\"/mnt/sdcard/Android/data/org.tuxpaint/files/fontconfig\" \
 -DFC_CACHEDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/fontconfig\" \
 -DFC_DEFAULT_FONTS=\"/system/fonts\" \
 -DHAVE_RANDOM_R=0 \
 -DHAVE_RANDOM=0 \
 -DHAVE_RAND=0 \
 -DSIZEOF_VOID_P=4 \
 -DFLEXIBLE_ARRAY_MEMBER \
 -DHAVE_MKSTEMP \
 -DHAVE_FCNTL_H=1 \
 -DFC_GPERF_SIZE_T=size_t \
 -DALIGNOF_VOID_P=8 \
 -DFC_TEMPLATEDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/fontconfig\" \
 -DHAVE_CONFIG_H

ifeq ($(TARGET_ARCH),arm)
 fontconfig_flags += -DALIGNOF_DOUBLE=8
else
 fontconfig_flags += -DALIGNOF_DOUBLE=4
endif

# fontconfig-2.11.1/config.h: comment out
# fontconfig-2.12.93/config.h: comment out
# HAVE_RANDOM_R
# HAVE_SYS_STATVFS_H
# uses uuid which is not available
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)


LOCAL_CFLAGS    := \
 $(fontconfig_flags)

LOCAL_C_INCLUDES := 				\
	$(LOCAL_PATH)/include			\
	$(LOCAL_PATH)/../libuuid111		\
	$(LOCAL_PATH)/../libintl-lite-0.5
#	external/freetype_x11/include	\
#	external/expat_x11/lib

LOCAL_SRC_FILES := \
    src/fcatomic.c \
    src/fccache.c \
    src/fccfg.c \
    src/fccharset.c \
    src/fccompat.c \
    src/fcdbg.c \
    src/fcdefault.c \
    src/fcdir.c \
    src/fcformat.c \
    src/fcfreetype.c \
    src/fcfs.c \
    src/fcptrlist.c \
    src/fchash.c \
    src/fcinit.c \
    src/fclang.c \
    src/fclist.c \
    src/fcmatch.c \
    src/fcmatrix.c \
    src/fcname.c \
    src/fcobjs.c \
    src/fcpat.c \
    src/fcrange.c \
    src/fcserialize.c \
    src/fcstat.c \
    src/fcstr.c \
    src/fcweight.c \
    src/fcxml.c \
    src/ftglue.c


LOCAL_SHARED_LIBRARIES := \
	tuxpaint_freetype \
	tuxpaint_iconv \
	tuxpaint_xml2 \
	tuxpaint_intl \
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
