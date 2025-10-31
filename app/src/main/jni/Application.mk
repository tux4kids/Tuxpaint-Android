APP_ABI      := arm64-v8a
APP_PLATFORM := android-16
APP_OPTIM    := release
APP_CFLAGS += -Wno-error=format-security -Wno-error=cast-function-type-strict -Wno-error -O3 -DENABLE_MULTITOUCH
APP_STL := c++_shared
APP_CPPFLAGS += -fexceptions -Wno-error=cast-function-type-strict -Wno-error
APP_CXXFLAGS += -Wno-error=cast-function-type-strict -Wno-error
APP_ALLOW_MISSING_DEPS  :=  true
# Add 16KB page size alignment support for Google Play Store 2025 requirements
APP_LDFLAGS += -Wl,-z,max-page-size=16384

