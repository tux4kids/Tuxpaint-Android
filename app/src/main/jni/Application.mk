APP_ABI      := arm64-v8a
APP_PLATFORM := android-16
APP_CFLAGS += -Wno-error=format-security -Wno-error=cast-function-type-strict -Wno-error -O3 -DENABLE_MULTITOUCH
APP_STL := c++_shared
APP_CPPFLAGS += -fexceptions
APP_ALLOW_MISSING_DEPS  :=  true

