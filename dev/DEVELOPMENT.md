# Performance Optimizations for Android

## 1. Native Code Optimizations (Application.mk)

### Changes in `app/src/main/jni/Application.mk`:
```makefile
APP_OPTIM    := release    # Forces release optimizations even for debug builds
APP_CFLAGS += -O3          # Maximum compiler optimizations
```

### Effect:
- **10-50x faster** than unoptimized debug code
- Works with debug builds too (for easier testing)
- Enables aggressive compiler optimizations (loop unrolling, inlining, etc.)

## Disable Logging

### For tests without logging:
```bash
# Completely disable logcat for org.tuxpaint.android
adb shell "setprop log.tag.TuxPaint SILENT"
adb shell "setprop log.tag.SDL SILENT"

# Or ERROR-level only
adb shell "setprop log.tag.TuxPaint ERROR"
```

### Reduce NDK logging:
In `Android.mk` or `Application.mk` you can add:
```makefile
APP_CFLAGS += -DNDEBUG              # Disables debug assertions
APP_CFLAGS += -Wno-unused-variable  # No warnings
```

## Build Variants

### Debug with optimizations (recommended for testing):
```bash
./gradlew assemblePlayStoreDebug
```
- Installable without signature
- With release optimizations (via APP_OPTIM=release)
- Fast enough for realistic performance tests

### Release Build:
```bash
./gradlew assemblePlayStoreRelease
```
- Must be signed before installation
- Smaller APK size
- Slightly faster than optimized debug build

## Expected Performance

With these optimizations, painting performance should now be:
- **Touch-to-screen latency**: < 50ms (instead of ~1000ms)
- **Continuous painting**: Smooth, no noticeable delays

## Important

Debug mode without optimization is **extremely slow** and not representative of real app performance!

## Applying Optimizations in Android Studio

### Method 1: Build Variants Panel (Recommended - Easiest)
**Best for: Most users**

1. Open **View** → **Tool Windows** → **Build Variants** (or click "Build Variants" tab on left side)
2. In the table, find your app module row
3. Click the dropdown under "Active Build Variant" column and select **playStoreDebug**
4. Click the **Run** button (green play icon) or press **Shift+F10** to build, install and run

The optimized variant is now active for all builds (manual or automatic).

### Method 2: Command Line
**Best for: Just building APK without running**

From Android Studio's Terminal tab at the bottom:
```bash
# Build the APK
./gradlew assemblePlayStoreDebug

# Then manually install and run:
adb install -r app/build/outputs/apk/playStore/debug/app-playStore-debug.apk
adb shell am start -n org.tuxpaint.android/org.tuxpaint.tuxpaintActivity
```

**Note:** This requires additional manual steps, so it's only recommended if you just need to build without running immediately.

### Method 4: Build Menu
**Best for: Building APKs for distribution**

1. Open **Build** → **Select Build Variant...**
2. Choose **playStoreDebug** or **playStoreRelease**
3. Click **Build** → **Build Bundle(s) / APK(s)** → **Build APK(s)**
4. Wait for build to complete
5. Click "locate" in the notification to find the APK
6. Drag & drop the APK onto the emulator window to install, or use adb install

### Verifying optimizations are active:
Check the build output for:
```
> Task :app:buildNdkBuildDebug[arm64-v8a]
```
The native libraries should be compiled with `-O3` flags (visible in verbose output).

### Troubleshooting in Android Studio:
- If build fails, try **Build** → **Clean Project**, then rebuild
- Check **Build** → **Make Project** to see compilation errors
- View full build log in the **Build** tab at the bottom
- For NDK issues, check **File** → **Project Structure** → **SDK Location** → NDK is installed
