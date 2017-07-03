LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_gdk_pixbuf

LOCAL_SRC_FILES :=  				\
	gdk-pixbuf/gdk-pixbuf.c		 	\
	gdk-pixbuf/gdk-pixbuf-animation.c	\
	gdk-pixbuf/gdk-pixbuf-data.c	 	\
	gdk-pixbuf/gdk-pixbuf-io.c		\
	gdk-pixbuf/gdk-pixbuf-loader.c	 	\
	gdk-pixbuf/gdk-pixbuf-scale.c	 	\
	gdk-pixbuf/gdk-pixbuf-simple-anim.c 	\
	gdk-pixbuf/gdk-pixbuf-scaled-anim.c 	\
	gdk-pixbuf/gdk-pixbuf-util.c	 	\
	gdk-pixbuf/gdk-pixdata.c		\
	gdk-pixbuf/gdk-pixbuf-enum-types.c	\
	gdk-pixbuf/io-pixdata.c			\
	gdk-pixbuf/io-png.c			\
	gdk-pixbuf/pixops/pixops.c		\
	$(NULL)

LOCAL_C_INCLUDES := 				\
	$(LOCAL_PATH)		           	\
	$(LOCAL_PATH)/gdk-pixbuf		\
	$(LOCAL_PATH)/gdk-pixbuf/pixops		\
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := 			\
	$(LOCAL_PATH)				\
	$(NULL)

# Although GDK_PIXBUF_LOCALEDIR and GDK_PIXBUF_LIBDIR are configured the place where TuxPaint lives,
# there is nothing in fact
LOCAL_CFLAGS := 							\
	-DG_LOG_DOMAIN=\"GdkPixbuf\"					\
	-DGDK_PIXBUF_COMPILATION					\
	-DGDK_PIXBUF_LOCALEDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/gdk-pixbuf\"	\
	-DGDK_PIXBUF_LIBDIR=\"/mnt/sdcard/Android/data/org.tuxpaint/files/gdk-pixbuf\"		\
	-DGDK_PIXBUF_BINARY_VERSION=\"2.10.0\"				\
	-DGDK_PIXBUF_ENABLE_BACKEND					\
	$(NULL)

LOCAL_SHARED_LIBRARIES := 	\
	tuxpaint_glib		\
	tuxpaint_intl		\
	tuxpaint_png		\
	tuxpaint_cairo		\
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
