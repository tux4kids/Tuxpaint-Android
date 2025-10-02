package org.tuxpaint;

import android.util.Log;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.fail;

/**
 * Instrumented test to reproduce EGL threading error
 * 
 * This test launches TuxPaint activity and waits to observe
 * the EGL_BAD_ACCESS threading error that occurs when running
 * on arm64-v8a Android emulator.
 * 
 * Expected error in logcat:
 * eglMakeCurrent: error: EGL_BAD_ACCESS: context current to another thread!
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class TuxpaintActivityTest {
    
    private static final String TAG = "TuxpaintActivityTest";
    
    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
            new ActivityScenarioRule<>(tuxpaintActivity.class);
    
    @Test
    public void testActivityLaunch_EGLThreadingError() {
        Log.d(TAG, "===== TEST: Launching TuxPaint activity to observe EGL threading behavior =====");
        
        // Activity is automatically launched by ActivityScenarioRule
        // Wait a few seconds to let the app initialize and trigger the EGL error
        try {
            Log.d(TAG, "Waiting 5 seconds to observe EGL errors...");
            Thread.sleep(5000);
            
            Log.d(TAG, "===== TEST: Check logcat for EGL_BAD_ACCESS errors =====");
            Log.d(TAG, "Expected error pattern:");
            Log.d(TAG, "  eglMakeCurrent: error: EGL_BAD_ACCESS: context current to another thread!");
            Log.d(TAG, "  tid XXXX: eglMakeCurrent(1976): error 0x3002 (EGL_BAD_ACCESS)");
            
            // Test passes if app doesn't crash (even with EGL errors)
            // The actual error diagnosis must be done by examining logcat output
            
        } catch (InterruptedException e) {
            fail("Test interrupted: " + e.getMessage());
        }
        
        Log.d(TAG, "===== TEST: Activity launched successfully (check logcat for EGL errors) =====");
    }
    
    @Test
    public void testActivityLifecycle_10Seconds() {
        Log.d(TAG, "===== TEST: Extended activity lifecycle test (10 seconds) =====");
        
        // Keep activity running longer to observe continuous EGL errors
        try {
            for (int i = 1; i <= 10; i++) {
                Thread.sleep(1000);
                Log.d(TAG, "Second " + i + "/10 - monitoring for EGL errors");
            }
        } catch (InterruptedException e) {
            fail("Test interrupted: " + e.getMessage());
        }
        
        Log.d(TAG, "===== TEST: Extended test complete =====");
    }
}
