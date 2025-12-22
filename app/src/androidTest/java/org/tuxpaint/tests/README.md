# Tuxpaint Android Tests

This directory contains instrumented tests for Tuxpaint Android.

## Test Files

### PaintingPerformanceTest.java
Tests painting responsiveness and performance.

**What it tests:**
- Touch-to-screen latency
- Continuous painting performance
- Event processing time

**Run with:**
```bash
./gradlew connectedPlayStoreDebugAndroidTest --tests "org.tuxpaint.tests.PaintingPerformanceTest"
```

### MultitouchTest.java
Tests multitouch painting functionality.

**What it tests:**
- Two-finger simultaneous painting
- Three-finger simultaneous painting
- Rapid touch add/remove (stress test)

**Run with:**
```bash
./gradlew connectedPlayStoreDebugAndroidTest --tests "org.tuxpaint.tests.MultitouchTest"
```

**Note:** Multitouch tests will FAIL until multitouch support is implemented (see `MULTITOUCH_IMPLEMENTATION.md`). The tests are written to verify the feature once it's implemented.

### SoundToggleTest.java
Tests sound toggle button functionality.

**What it tests:**
- Sound button click detection
- Mute state toggling
- Sound blocking when muted
- Sound playback when unmuted

**Run with:**
```bash
./gradlew connectedOffPlayStoreDebugAndroidTest -Pandroid.testInstrumentationRunnerArguments.class=org.tuxpaint.tests.SoundToggleTest
```

## Running All Tests

```bash
# Run all instrumented tests
./gradlew connectedPlayStoreDebugAndroidTest

# Run specific test class
./gradlew connectedPlayStoreDebugAndroidTest --tests "org.tuxpaint.tests.MultitouchTest"

# Run specific test method
./gradlew connectedPlayStoreDebugAndroidTest --tests "org.tuxpaint.tests.MultitouchTest.testTwoFingerSimultaneousPainting"
```

## Viewing Test Results

After running tests, view the report at:
```
app/build/reports/androidTests/connected/index.html
```

Or check logcat for detailed output:
```bash
adb logcat -s MultitouchTest PaintPerformance
```

## Test Requirements

- Android emulator or device connected
- App installed in debug mode
- Minimum Android API 21

## Test Structure

All tests:
- Wait 5 seconds for app to load
- Use `ActivityScenarioRule` for activity lifecycle
- Log detailed information with unique tags
- Use assertions to verify behavior

## Adding New Tests

1. Create new test class in this directory
2. Use package: `package org.tuxpaint.tests;`
3. Import activity: `import org.tuxpaint.tuxpaintActivity;`
4. Annotate with `@RunWith(AndroidJUnit4.class)`
5. Add `@Rule` for ActivityScenarioRule
6. Write test methods with `@Test` annotation
