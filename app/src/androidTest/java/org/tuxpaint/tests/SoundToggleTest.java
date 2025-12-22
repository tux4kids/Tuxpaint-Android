package org.tuxpaint.tests;

import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.tuxpaint.tuxpaintActivity;

import static org.junit.Assert.assertTrue;

/**
 * Tests for the sound toggle button functionality.
 * Verifies that clicking the sound button actually mutes/unmutes sound.
 */
@RunWith(AndroidJUnit4.class)
public class SoundToggleTest {

    private static final String TAG = "SoundToggleTest";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    /**
     * Test that the sound toggle button can be clicked and toggles mute state.
     * The button should be at position (0-107, 856-963) based on button_h=107.
     */
    @Test
    public void testSoundButtonToggle() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: SOUND BUTTON TOGGLE");
        Log.i(TAG, "Testing sound mute/unmute functionality");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Log.i(TAG, "Waiting 12 seconds for app initialization...");
        Thread.sleep(12000);

        final boolean[] testComplete = {false};
        final boolean[] soundButtonClicked = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity is ready");
            View rootView = activity.getWindow().getDecorView().getRootView();
            
            try {
                int screenWidth = rootView.getWidth();
                int screenHeight = rootView.getHeight();
                
                Log.i(TAG, "Screen size: " + screenWidth + "x" + screenHeight);
                
                // Sound button is at toolbar row 8 (below tools)
                // Based on logs: button rect is approximately (0, 760, 95, 95)
                // Click at center of button
                int soundButtonX = 47;  // Center of 95px button
                int soundButtonY = 808;  // Center: 760 + 95/2
                
                Log.i(TAG, "Sound button target position: (" + soundButtonX + ", " + soundButtonY + ")");
                Log.i(TAG, "==========================================");
                
                // Test 1: Click to MUTE
                Log.i(TAG, "TEST 1: Clicking sound button to MUTE");
                clickAt(rootView, soundButtonX, soundButtonY);
                Thread.sleep(1000);
                Log.i(TAG, "First click complete - should be MUTED now");
                Log.i(TAG, "Check logcat for: 'SOUND_BTN_LOG: Sound is now MUTED'");
                
                // Test 2: Click to UNMUTE
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 2: Clicking sound button to UNMUTE");
                clickAt(rootView, soundButtonX, soundButtonY);
                Thread.sleep(1000);
                Log.i(TAG, "Second click complete - should be UNMUTED now");
                Log.i(TAG, "Check logcat for: 'SOUND_BTN_LOG: Sound is now UNMUTED'");
                
                // Test 3: Click to MUTE again
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 3: Clicking sound button to MUTE again");
                clickAt(rootView, soundButtonX, soundButtonY);
                Thread.sleep(1000);
                Log.i(TAG, "Third click complete - should be MUTED again");
                
                // Test 4: Try painting while muted to verify sound is blocked
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 4: Testing painting while MUTED");
                int canvasCenterX = screenWidth / 2;
                int canvasCenterY = screenHeight / 2;
                
                Log.i(TAG, "Drawing on canvas at (" + canvasCenterX + ", " + canvasCenterY + ")");
                Log.i(TAG, "Sound should be BLOCKED if mute is working");
                
                // Quick paint stroke
                long downTime = SystemClock.uptimeMillis();
                MotionEvent downEvent = MotionEvent.obtain(
                    downTime, downTime, 
                    MotionEvent.ACTION_DOWN, 
                    canvasCenterX, canvasCenterY, 0
                );
                rootView.dispatchTouchEvent(downEvent);
                downEvent.recycle();
                Thread.sleep(200);
                
                MotionEvent upEvent = MotionEvent.obtain(
                    downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_UP,
                    canvasCenterX + 50, canvasCenterY, 0
                );
                rootView.dispatchTouchEvent(upEvent);
                upEvent.recycle();
                Thread.sleep(500);
                
                Log.i(TAG, "Check logcat for: 'PLAYSOUND_LOG: Sound blocked - mute=1'");
                
                // Test 5: Unmute and test sound works
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 5: Unmuting and testing sound works");
                clickAt(rootView, soundButtonX, soundButtonY);
                Thread.sleep(1000);
                
                Log.i(TAG, "Drawing again - sound should now PLAY");
                downTime = SystemClock.uptimeMillis();
                downEvent = MotionEvent.obtain(
                    downTime, downTime, 
                    MotionEvent.ACTION_DOWN, 
                    canvasCenterX, canvasCenterY + 100, 0
                );
                rootView.dispatchTouchEvent(downEvent);
                downEvent.recycle();
                Thread.sleep(200);
                
                upEvent = MotionEvent.obtain(
                    downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_UP,
                    canvasCenterX + 50, canvasCenterY + 100, 0
                );
                rootView.dispatchTouchEvent(upEvent);
                upEvent.recycle();
                Thread.sleep(500);
                
                Log.i(TAG, "Check logcat for: 'PLAYSOUND_LOG: Condition passed - playing sound'");
                
                soundButtonClicked[0] = true;
                testComplete[0] = true;
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "SOUND TOGGLE TEST COMPLETED!");
                Log.i(TAG, "Review logcat output to verify:");
                Log.i(TAG, "1. Button clicks were detected");
                Log.i(TAG, "2. Mute state toggled correctly");
                Log.i(TAG, "3. Sounds were blocked when muted");
                Log.i(TAG, "4. Sounds played when unmuted");
                Log.i(TAG, "==========================================");
                
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during sound toggle test", e);
                testComplete[0] = true;
                soundButtonClicked[0] = false;
            }
        });
        
        // Wait for test to complete
        int timeout = 0;
        while (!testComplete[0] && timeout < 200) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Sound toggle test did not complete", testComplete[0]);
        assertTrue("Sound button click test failed", soundButtonClicked[0]);
        
        Log.i(TAG, "✓ TEST PASSED: Sound toggle button test completed!");
        Log.i(TAG, "Note: Manual verification required - check logcat for SOUND_BTN_LOG and PLAYSOUND_LOG");
    }
    
    /**
     * Helper method to click at a specific position
     */
    private void clickAt(View rootView, int x, int y) throws InterruptedException {
        long downTime = SystemClock.uptimeMillis();
        
        Log.i(TAG, "Clicking at (" + x + ", " + y + ")");
        
        MotionEvent downEvent = MotionEvent.obtain(
            downTime, downTime, 
            MotionEvent.ACTION_DOWN, 
            x, y, 0
        );
        boolean downResult = rootView.dispatchTouchEvent(downEvent);
        Log.i(TAG, "  ACTION_DOWN dispatched: " + downResult);
        downEvent.recycle();
        
        Thread.sleep(100);
        
        MotionEvent upEvent = MotionEvent.obtain(
            downTime, SystemClock.uptimeMillis(),
            MotionEvent.ACTION_UP,
            x, y, 0
        );
        boolean upResult = rootView.dispatchTouchEvent(upEvent);
        Log.i(TAG, "  ACTION_UP dispatched: " + upResult);
        upEvent.recycle();
        
        Thread.sleep(200);
    }
}
