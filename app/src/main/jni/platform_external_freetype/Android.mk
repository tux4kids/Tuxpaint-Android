# this is now the default FreeType build for Android
#
ifndef USE_FREETYPE
USE_FREETYPE := 2.10.1
endif

ifeq ($(USE_FREETYPE),2.10.1)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# compile in ARM mode, since the glyph loader/renderer is a hotspot
# when loading complex pages in the browser
#
LOCAL_ARM_MODE := arm

# Adapted from INSTALL.ANY in the docs directory
#    -- base components (required)
LOCAL_SRC_FILES := \
      src/base/ftsystem.c			\
      src/base/ftinit.c				\
      src/base/ftdebug.c			\
      src/base/ftbase.c				\
      src/base/ftbbox.c    			\
      src/base/ftglyph.c   			\
      src/base/ftbdf.c      			\
      src/base/ftbitmap.c   			\
      src/base/ftcid.c      			\
      src/base/ftfstype.c   			\
      src/base/ftgasp.c     			\
      src/base/ftgxval.c    			\
      src/base/ftmm.c       			\
      src/base/ftotval.c    			\
      src/base/ftpatent.c   			\
      src/base/ftpfr.c      			\
      src/base/ftstroke.c   			\
      src/base/ftsynth.c    			\
      src/base/fttype1.c    			\
      src/base/ftwinfnt.c   


#    -- font drivers (optional; at least one is needed)
LOCAL_SRC_FILES += \
      src/bdf/bdf.c        			\
      src/cff/cff.c        			\
      src/cid/type1cid.c   			\
      src/pcf/pcf.c        			\
      src/pfr/pfr.c        			\
      src/sfnt/sfnt.c      			\
      src/truetype/truetype.c			\
      src/type1/type1.c    			\
      src/type42/type42.c  			\
      src/winfonts/winfnt.c


#    -- rasterizers (optional; at least one is needed for vector
#       formats)
#      src/raster/raster.c     -- monochrome rasterizer
LOCAL_SRC_FILES += \
      src/smooth/smooth.c \
      src/raster/ftrend1.c \
      src/raster/ftraster.c


#    -- auxiliary modules (optional)
LOCAL_SRC_FILES += \
      src/autofit/autofit.c   			\
      src/cache/ftcache.c     			\
      src/gzip/ftgzip.c       			\
      src/lzw/ftlzw.c         			\
      src/bzip2/ftbzip2.c     			\
      src/gxvalid/gxvalid.c   			\
      src/otvalid/otvalid.c   			\
      src/psaux/psaux.c       			\
      src/pshinter/pshinter.c 			\
      src/psnames/psnames.c   



#LOCAL_C_INCLUDES += \
#    $(LOCAL_PATH)/include \
#    external/libpng \
#    external/zlib
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_CFLAGS += -W -Wall
LOCAL_CFLAGS += -fPIC -DPIC
LOCAL_CFLAGS += "-DDARWIN_NO_CARBON"
LOCAL_CFLAGS += "-DFT2_BUILD_LIBRARY"

#LOCAL_SHARED_LIBRARIES += libpng libz
LOCAL_SHARED_LIBRARIES += tuxpaint_png libz

# the following is for testing only, and should not be used in final builds
# of the product
#LOCAL_CFLAGS += "-DTT_CONFIG_OPTION_BYTECODE_INTERPRETER"

LOCAL_CFLAGS += -O2

#LOCAL_MODULE:= libft2
LOCAL_MODULE:= tuxpaint_freetype

include $(BUILD_SHARED_LIBRARY)
endif
