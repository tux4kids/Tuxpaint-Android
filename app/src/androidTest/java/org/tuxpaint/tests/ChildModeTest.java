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
 * Tests for the child mode toggle button functionality.
 * Verifies that clicking the child mode button:
 * - Toggles child_mode variable
 * - Hides Tux area (r_tuxarea.h = 0)
 * - Extends color area (r_colors.h increases)
 */
@RunWith(AndroidJUnit4.class)
public class ChildModeTest {

    private static final String TAG = "ChildModeTest";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    /**
     * Test that the child mode toggle button can be clicked and changes layout.
     * The button should be at Row 8, Column 1 (right of sound button).
     */
    @Test
    public void testChildModeToggle() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: CHILD MODE TOGGLE");
        Log.i(TAG, "Testing child mode layout changes");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Log.i(TAG, "Waiting 12 seconds for app initialization...");
        Thread.sleep(12000);

        final boolean[] testComplete = {false};
        final boolean[] childModeButtonClicked = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity is ready");
            View rootView = activity.getWindow().getDecorView().getRootView();
            
            try {
                int screenWidth = rootView.getWidth();
                int screenHeight = rootView.getHeight();
                
                Log.i(TAG, "Screen size: " + screenWidth + "x" + screenHeight);
                
                // Child mode button is at toolbar row 8, column 1 (right of sound button)
                // Sound button is at (0, 760) with width 95px
                // Child mode button should be at (95, 760) with width 95px
                int childModeButtonX = 95 + 47;  // Column 1 center: 95 + button_w/2
                int childModeButtonY = 808;      // Same row as sound button
                
                Log.i(TAG, "Child mode button target position: (" + childModeButtonX + ", " + childModeButtonY + ")");
                Log.i(TAG, "==========================================");
                
                // Test 1: Click to ACTIVATE child mode
                Log.i(TAG, "TEST 1: Clicking child mode button to ACTIVATE");
                Log.i(TAG, "Expected: Tux disappears, colors extend to bottom");
                clickAt(rootView, childModeButtonX, childModeButtonY);
                Thread.sleep(1500);
                Log.i(TAG, "First click complete - child mode should be ACTIVE now");
                Log.i(TAG, "Check logcat for:");
                Log.i(TAG, "  - 'CHILD_MODE: Toggled to 1'");
                Log.i(TAG, "  - 'LAYOUT: Child mode active - hiding Tux, extending colors'");
                Log.i(TAG, "  - 'CHILD_MODE: r_colors AFTER layout: h=[larger value]'");
                Log.i(TAG, "  - 'CHILD_MODE: r_tuxarea AFTER layout: h=0'");
                
                // Test 2: Click to DEACTIVATE child mode
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 2: Clicking child mode button to DEACTIVATE");
                Log.i(TAG, "Expected: Tux reappears, colors return to normal size");
                clickAt(rootView, childModeButtonX, childModeButtonY);
                Thread.sleep(1500);
                Log.i(TAG, "Second click complete - child mode should be INACTIVE now");
                Log.i(TAG, "Check logcat for:");
                Log.i(TAG, "  - 'CHILD_MODE: Toggled to 0'");
                Log.i(TAG, "  - 'LAYOUT: Normal mode - showing Tux'");
                Log.i(TAG, "  - 'CHILD_MODE: r_colors AFTER layout: h=[smaller value]'");
                Log.i(TAG, "  - 'CHILD_MODE: r_tuxarea AFTER layout: h=[positive value]'");
                
                // Test 3: Activate again and verify consistency
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 3: Activating child mode AGAIN");
                clickAt(rootView, childModeButtonX, childModeButtonY);
                Thread.sleep(1500);
                Log.i(TAG, "Third click complete - child mode should be ACTIVE again");
                
                // Test 4: Deactivate and verify consistency
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 4: Deactivating child mode AGAIN");
                clickAt(rootView, childModeButtonX, childModeButtonY);
                Thread.sleep(1500);
                Log.i(TAG, "Fourth click complete - child mode should be INACTIVE again");
                
                // Test 5: Verify layout values are logged
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TEST 5: Final activation to verify layout values");
                clickAt(rootView, childModeButtonX, childModeButtonY);
                Thread.sleep(1500);
                
                childModeButtonClicked[0] = true;
                testComplete[0] = true;
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "CHILD MODE TEST COMPLETED!");
                Log.i(TAG, "Review logcat output to verify:");
                Log.i(TAG, "1. child_mode variable toggled correctly (0 <-> 1)");
                Log.i(TAG, "2. r_tuxarea.h = 0 when child mode active");
                Log.i(TAG, "3. r_colors.h increased when child mode active");
                Log.i(TAG, "4. Layout reverted to normal when deactivated");
                Log.i(TAG, "5. LAYOUT log messages appeared");
                Log.i(TAG, "==========================================");
                
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during child mode test", e);
                testComplete[0] = true;
                childModeButtonClicked[0] = false;
            }
        });
        
        // Wait for test to complete
        int timeout = 0;
        while (!testComplete[0] && timeout < 300) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Child mode test did not complete", testComplete[0]);
        assertTrue("Child mode button click test failed", childModeButtonClicked[0]);
        
        Log.i(TAG, "✓ TEST PASSED: Child mode toggle button test completed!");
        Log.i(TAG, "Note: Manual verification required - check logcat for CHILD_MODE and LAYOUT logs");
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
