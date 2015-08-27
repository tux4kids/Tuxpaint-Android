LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tuxpaint_cairo 

LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H \
	-D_REENTRANT \
	-DCAIRO_NO_MUTEX=1 \
	-Wno-attributes \
	$(NULL)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/src \
	$(NULL)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(NULL)

SRC_FILES :=  \
	cairo-analysis-surface.c \
	cairo-arc.c \
	cairo-array.c \
	cairo-atomic.c \
	cairo-base64-stream.c \
	cairo-base85-stream.c \
	cairo-bentley-ottmann.c \
	cairo-bentley-ottmann-rectangular.c \
	cairo-bentley-ottmann-rectilinear.c \
	cairo-botor-scan-converter.c \
	cairo-boxes.c \
	cairo-boxes-intersect.c \
	cairo.c \
	cairo-cache.c \
	cairo-clip.c \
	cairo-clip-boxes.c \
	cairo-clip-polygon.c \
	cairo-clip-region.c \
	cairo-clip-surface.c \
	cairo-color.c \
	cairo-composite-rectangles.c \
	cairo-compositor.c \
	cairo-contour.c \
	cairo-damage.c \
	cairo-debug.c \
	cairo-default-context.c \
	cairo-device.c \
	cairo-error.c \
	cairo-fallback-compositor.c \
	cairo-fixed.c \
	cairo-font-face.c \
	cairo-font-face-twin.c \
	cairo-font-face-twin-data.c \
	cairo-font-options.c \
	cairo-freelist.c \
	cairo-freed-pool.c \
	cairo-gstate.c \
	cairo-hash.c \
	cairo-hull.c \
	cairo-image-compositor.c \
	cairo-image-info.c \
	cairo-image-source.c \
	cairo-image-surface.c \
	cairo-line.c \
	cairo-lzw.c \
	cairo-matrix.c \
	cairo-mask-compositor.c \
	cairo-mesh-pattern-rasterizer.c \
	cairo-mempool.c \
	cairo-misc.c \
	cairo-mono-scan-converter.c \
	cairo-mutex.c \
	cairo-no-compositor.c \
	cairo-observer.c \
	cairo-output-stream.c \
	cairo-paginated-surface.c \
	cairo-path-bounds.c \
	cairo-path.c \
	cairo-path-fill.c \
	cairo-path-fixed.c \
	cairo-path-in-fill.c \
	cairo-path-stroke.c \
	cairo-path-stroke-boxes.c \
	cairo-path-stroke-polygon.c \
	cairo-path-stroke-traps.c \
	cairo-path-stroke-tristrip.c \
	cairo-pattern.c \
	cairo-pen.c \
	cairo-polygon.c \
	cairo-polygon-intersect.c \
	cairo-polygon-reduce.c \
	cairo-raster-source-pattern.c \
	cairo-recording-surface.c \
	cairo-rectangle.c \
	cairo-rectangular-scan-converter.c \
	cairo-region.c \
	cairo-rtree.c \
	cairo-scaled-font.c \
	cairo-shape-mask-compositor.c \
	cairo-slope.c \
	cairo-spans.c \
	cairo-spans-compositor.c \
	cairo-spline.c \
	cairo-stroke-dash.c \
	cairo-stroke-style.c \
	cairo-surface.c \
	cairo-surface-clipper.c \
	cairo-surface-fallback.c \
	cairo-surface-observer.c \
	cairo-surface-offset.c \
	cairo-surface-snapshot.c \
	cairo-surface-subsurface.c \
	cairo-surface-wrapper.c \
	cairo-time.c \
	cairo-tor-scan-converter.c \
	cairo-tor22-scan-converter.c \
	cairo-clip-tor-scan-converter.c \
	cairo-toy-font-face.c \
	cairo-traps.c \
	cairo-tristrip.c \
	cairo-traps-compositor.c \
	cairo-unicode.c \
	cairo-user-font.c \
	cairo-version.c \
	cairo-wideint.c \
	cairo-cff-subset.c \
	cairo-scaled-font-subsets.c \
	cairo-truetype-subset.c \
	cairo-type1-fallback.c \
	cairo-type1-glyph-names.c \
	cairo-type1-subset.c \
	cairo-png.c \
	cairo-ft-font.c \
	cairo-svg-surface.c \
	$(NULL)

LOCAL_SRC_FILES := $(addprefix src/, $(SRC_FILES))

LOCAL_SHARED_LIBRARIES := \
	tuxpaint_pixman \
	tuxpaint_freetype \
	tuxpaint_fontconfig \
	tuxpaint_png \
	$(NULL)

include $(BUILD_SHARED_LIBRARY)
