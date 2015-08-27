LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_fontconfig

# Although FC_CACHEDIR and FONTCONFIG_PATH is configured to the place where TuxPaint lives, there is nothing in fact. 
LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H \
	-DFC_CACHEDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/fontconfig\" \
	-DFONTCONFIG_PATH=\"/mnt/sdcard/Android/data/org.tuxpaint/files/fontconfig\" \
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

SRC_FILES :=  \
	fcatomic.c \
	fcblanks.c \
	fccache.c \
	fccfg.c \
	fccharset.c \
	fcdbg.c \
	fcdefault.c \
	fcdir.c \
	fcformat.c \
	fcfreetype.c \
	fcfs.c \
	fcinit.c \
	fclang.c \
	fclist.c \
	fcmatch.c \
	fcmatrix.c \
	fcname.c \
	fcpat.c \
	fcserialize.c \
	fcstr.c \
	fcxml.c \
	ftglue.c \
	$(NULL)

LOCAL_SRC_FILES := $(addprefix src/, $(SRC_FILES))

LOCAL_SHARED_LIBRARIES := \
	tuxpaint_freetype \
	tuxpaint_iconv \
	tuxpaint_xml2 \
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
