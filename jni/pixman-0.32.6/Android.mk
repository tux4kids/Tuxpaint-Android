LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_pixman

LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H \
	-DPIXMAN_NO_TLS \
	-D_USE_MATH_DEFINES \
	$(NULL)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/pixman

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/pixman

SRC_FILES :=  \
	pixman.c			\
	pixman-access.c			\
	pixman-access-accessors.c	\
	pixman-bits-image.c		\
	pixman-combine32.c		\
	pixman-combine-float.c		\
	pixman-conical-gradient.c	\
	pixman-filter.c			\
	pixman-x86.c			\
	pixman-mips.c			\
	pixman-arm.c			\
	pixman-ppc.c			\
	pixman-edge.c			\
	pixman-edge-accessors.c		\
	pixman-fast-path.c		\
	pixman-glyph.c			\
	pixman-general.c		\
	pixman-gradient-walker.c	\
	pixman-image.c			\
	pixman-implementation.c		\
	pixman-linear-gradient.c	\
	pixman-matrix.c			\
	pixman-noop.c			\
	pixman-radial-gradient.c	\
	pixman-region16.c		\
	pixman-region32.c		\
	pixman-solid-fill.c		\
	pixman-timer.c			\
	pixman-trap.c			\
	pixman-utils.c			\
	$(NULL)

LOCAL_SRC_FILES := $(addprefix pixman/, $(SRC_FILES))

include $(BUILD_SHARED_LIBRARY)
