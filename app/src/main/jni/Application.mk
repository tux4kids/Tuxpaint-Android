APP_ABI      := armeabi armeabi-v7a
APP_PLATFORM := android-16
APP_CFLAGS += -Wno-error=format-security
APP_STL := c++_shared
APP_CPPFLAGS += -fexceptions
APP_ALLOW_MISSING_DEPS  :=  true

# Note: Uncomment the next line to build with ndk r14b in order to get older Androids support
# NDK_TOOLCHAIN_VERSION := 4.9
