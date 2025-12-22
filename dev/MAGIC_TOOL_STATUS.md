# MAGIC Tool Status

**Date:** 2025-10-09 22:30  
**Status:** MAGIC tool disabled - plugins not included in APK

## Problem

The MAGIC tool is disabled because no magic plugins are being loaded:

```c
// tuxpaint.c line 14740
if (num_magics_total == 0)
    tool_avail[TOOL_MAGIC] = 0;
```

## Investigation

### Magic Plugin Source Files Exist

Location: `/app/src/main/jni/tuxpaint/magic/src/`

Available plugins include:
- `alien.c` - Alien effect
- `ascii.c` - ASCII art conversion
- `blur.c` - Blur effect
- `emboss.c` - Emboss effect
- `kaleidoscope.c` - Kaleidoscope effect
- `noise.c` - Noise effect
- `ripples.c` - Ripple effect
- ... and 60+ more

### Compiled Libraries Found

Location: `/app/src/main/obj/local/arm64-v8a/`

Compiled `.so` files exist:
- `libkaleidoscope.so`
- `libemboss.so`
- `libnoise.so`
- `libbloom.so`
- `libripples.so`
- ... etc.

### Root Cause

**The magic plugin `.so` files are NOT being included in the APK!**

- No `/app/src/main/jniLibs/arm64-v8a/` directory
- No `Android.mk` file in `/magic/` directory
- Plugins compile but aren't packaged

## Solution

To enable MAGIC tools:

### Option 1: Add Android.mk for Magic Plugins

Create `/app/src/main/jni/tuxpaint/magic/Android.mk`:

```makefile
LOCAL_PATH := $(call my-dir)

# List all magic tool source files
MAGIC_SOURCES := $(wildcard $(LOCAL_PATH)/src/*.c)

# Build each magic tool as a separate shared library
define build-magic-plugin
include $(CLEAR_VARS)
LOCAL_MODULE := $(basename $(notdir $(1)))
LOCAL_SRC_FILES := src/$(notdir $(1))
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src
LOCAL_SHARED_LIBRARIES := SDL2 tuxpaint_cairo tuxpaint_pango
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
endef

$(foreach src,$(MAGIC_SOURCES),$(eval $(call build-magic-plugin,$(src))))
```

### Option 2: Copy Compiled Libraries

Copy all `.so` files from `obj/local/arm64-v8a/lib*.so` to `jniLibs/arm64-v8a/`:

```bash
mkdir -p app/src/main/jniLibs/arm64-v8a/
cp app/src/main/obj/local/arm64-v8a/lib*.so app/src/main/jniLibs/arm64-v8a/
```

### Option 3: Gradle Configuration

Modify `app/build.gradle` to include magic plugins:

```gradle
sourceSets {
    main {
        jniLibs.srcDirs = ['src/main/obj/local']
    }
}
```

## Testing

Once enabled, test with:

```bash
bash test_magic_tool.sh
```

Expected behavior:
1. MAGIC tool visible in toolbar
2. Click MAGIC → magic effects list appears
3. Select effect (e.g., Blur, Ripples)
4. Click/drag on canvas → effect applied

## Current Status (Updated 2025-10-09 22:40)

- ✅ Source code present
- ✅ Compiles successfully  
- ✅ **62 plugin `.so` files packaged in APK**
- ✅ **Plugins initialize successfully**
- ✅ **Plugins report tool counts (perspective: 5, rainbow: 3, etc.)**
- ❌ **Icons not in APK** → `get_icon()` returns NULL → tools not registered
- ❌ MAGIC tool still disabled (0 tools loaded)

### Root Cause Identified

The magic plugin libraries ARE in the APK and DO initialize. However:

1. Plugin calls `get_icon()` to load its icon
2. Icons are in `/magic/icons/` in source but NOT in APK assets
3. `get_icon()` returns NULL
4. Code requires icon != NULL to register tool (line 24101)
5. Result: `num_magics_total = 0`

**Log Evidence:**
```
Magic plugin libemboss.so: init returned 1
Magic plugin libemboss.so: tool_count = 1
[but icon fails to load, so tool not registered]
...
Loaded 0 magic tools from 62 plug-in files
```

## Next Steps

1. Create Android.mk for magic plugins
2. Or copy libraries to jniLibs
3. Rebuild APK
4. Test MAGIC tool functionality

---

**Note:** The TEXT tool issue is separate and partially fixed. MAGIC tool is a packaging/build issue, not a code issue.
