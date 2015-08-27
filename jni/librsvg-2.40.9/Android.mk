LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_rsvg

LOCAL_SRC_FILES :=  			\
	gdk-pixbuf-loader/io-svg.c	\
	librsvg-features.c 	\
	rsvg-css.c 		\
	rsvg-defs.c 		\
	rsvg-image.c		\
	rsvg-io.c		\
	rsvg-paint-server.c 	\
	rsvg-path.c 		\
	rsvg-base-file-util.c 	\
	rsvg-filter.c		\
	rsvg-marker.c		\
	rsvg-mask.c		\
	rsvg-shapes.c		\
	rsvg-structure.c	\
	rsvg-styles.c		\
	rsvg-text.c		\
	rsvg-cond.c		\
	rsvg-base.c		\
	librsvg-enum-types.c	\
	rsvg-cairo-draw.c	\
	rsvg-cairo-render.c	\
	rsvg-cairo-clip.c	\
	rsvg.c			\
	rsvg-gobject.c		\
	rsvg-file-util.c	\
	rsvg-size-callback.c	\
	rsvg-xml.c		\
	$(NULL)

LOCAL_C_INCLUDES := 				\
	$(LOCAL_PATH)		           	\
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := 			\
	$(LOCAL_PATH)/include	           	\
	$(NULL)

# ./glib private macros, copy from Makefile.am
LOCAL_CFLAGS := \
	-DG_LOG_DOMAIN=\"librsvg\" 		\
	-DRSVG_DISABLE_DEPRECATION_WARNINGS	\
	-DHAVE_CONFIG_H				\
	-DRSVG_COMPILATION			\
	-DGDK_PIXBUF_ENABLE_BACKEND		\
	$(NULL)

LOCAL_SHARED_LIBRARIES := 	\
	tuxpaint_glib		\
	tuxpaint_intl		\
	tuxpaint_ffi		\
	tuxpaint_gdk_pixbuf	\
	tuxpaint_pango		\
	tuxpaint_croco		\
	tuxpaint_xml2		\
	tuxpaint_cairo		\
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
