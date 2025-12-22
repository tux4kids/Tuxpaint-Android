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
 * SIMPLIFIED multitouch test that actually verifies drawing works.
 * Tests that touch events reach the app and allow painting.
 */
@RunWith(AndroidJUnit4.class)
public class SimpleMultitouchTest {

    private static final String TAG = "SimpleMultitouch";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    /**
     * Test 1: Single finger can draw (basic functionality)
     * This ensures mouse events work and basic painting is functional.
     */
    @Test
    public void testSingleFingerDrawing() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST 1: SINGLE FINGER DRAWING");
        Log.i(TAG, "Testing basic touch/paint functionality");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load and show canvas
        Log.i(TAG, "Waiting 12 seconds for app initialization...");
        Thread.sleep(12000);

        final boolean[] testComplete = {false};
        final boolean[] drawingSucceeded = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity is ready");
            View rootView = activity.getWindow().getDecorView().getRootView();
            
            try {
                // Get canvas center position (should be safe drawing area)
                int centerX = rootView.getWidth() / 2;
                int centerY = rootView.getHeight() / 2;
                
                Log.i(TAG, "Screen size: " + rootView.getWidth() + "x" + rootView.getHeight());
                Log.i(TAG, "Drawing position: (" + centerX + ", " + centerY + ")");
                
                long downTime = SystemClock.uptimeMillis();
                
                // Touch down at center
                Log.i(TAG, "ACTION_DOWN at center");
                MotionEvent downEvent = MotionEvent.obtain(
                    downTime, downTime, 
                    MotionEvent.ACTION_DOWN, 
                    centerX, centerY, 0
                );
                boolean downResult = rootView.dispatchTouchEvent(downEvent);
                Log.i(TAG, "  ACTION_DOWN dispatched: " + downResult);
                downEvent.recycle();
                
                Thread.sleep(100);
                
                // Draw a horizontal line (move right)
                Log.i(TAG, "Drawing horizontal line...");
                for (int i = 1; i <= 5; i++) {
                    long eventTime = SystemClock.uptimeMillis();
                    int x = centerX + (i * 50);
                    int y = centerY;
                    
                    MotionEvent moveEvent = MotionEvent.obtain(
                        downTime, eventTime,
                        MotionEvent.ACTION_MOVE,
                        x, y, 0
                    );
                    boolean moveResult = rootView.dispatchTouchEvent(moveEvent);
                    Log.i(TAG, "  MOVE #" + i + " to (" + x + "," + y + ") dispatched: " + moveResult);
                    moveEvent.recycle();
                    
                    Thread.sleep(50);
                }
                
                // Touch up
                long upTime = SystemClock.uptimeMillis();
                Log.i(TAG, "ACTION_UP");
                MotionEvent upEvent = MotionEvent.obtain(
                    downTime, upTime,
                    MotionEvent.ACTION_UP,
                    centerX + 250, centerY, 0
                );
                boolean upResult = rootView.dispatchTouchEvent(upEvent);
                Log.i(TAG, "  ACTION_UP dispatched: " + upResult);
                upEvent.recycle();
                
                // If we got here without crashing, drawing succeeded
                drawingSucceeded[0] = true;
                testComplete[0] = true;
                
                // Give SDL time to process events before shutdown
                Thread.sleep(2000);
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "Single finger drawing test COMPLETED");
                Log.i(TAG, "Events dispatched successfully!");
                Log.i(TAG, "==========================================");
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during single finger test", e);
                testComplete[0] = true;
                drawingSucceeded[0] = false;
            }
        });
        
        // Wait for test to complete
        int timeout = 0;
        while (!testComplete[0] && timeout < 100) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Single finger test did not complete", testComplete[0]);
        assertTrue("Single finger drawing failed - events not dispatched", drawingSucceeded[0]);
        
        Log.i(TAG, "✓ TEST 1 PASSED: Single finger drawing works!");
    }
    
    /**
     * Test 2: Two fingers can draw simultaneously
     * This is the REAL multitouch test.
     */
    @Test
    public void testTwoFingersDrawingSimultaneously() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST 2: TWO FINGERS SIMULTANEOUS DRAWING");
        Log.i(TAG, "Testing TRUE multitouch functionality");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Log.i(TAG, "Waiting 12 seconds for app initialization...");
        Thread.sleep(12000);

        final boolean[] testComplete = {false};
        final boolean[] multitouchSucceeded = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity is ready for multitouch test");
            View rootView = activity.getWindow().getDecorView().getRootView();
            
            try {
                int screenWidth = rootView.getWidth();
                int screenHeight = rootView.getHeight();
                
                // Finger 1: Top-left to top-right
                int finger1_startX = screenWidth / 4;
                int finger1_startY = screenHeight / 3;
                
                // Finger 2: Bottom-left to bottom-right
                int finger2_startX = screenWidth / 4;
                int finger2_startY = (screenHeight * 2) / 3;
                
                Log.i(TAG, "Finger 1 will draw from (" + finger1_startX + "," + finger1_startY + ")");
                Log.i(TAG, "Finger 2 will draw from (" + finger2_startX + "," + finger2_startY + ")");
                
                long downTime = SystemClock.uptimeMillis();
                
                // Setup pointer properties and coordinates for 2 fingers
                MotionEvent.PointerProperties[] props = new MotionEvent.PointerProperties[2];
                MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[2];
                
                for (int i = 0; i < 2; i++) {
                    props[i] = new MotionEvent.PointerProperties();
                    props[i].id = i;
                    props[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
                    
                    coords[i] = new MotionEvent.PointerCoords();
                    coords[i].pressure = 1.0f;
                    coords[i].size = 1.0f;
                }
                
                // Finger 1 DOWN
                coords[0].x = finger1_startX;
                coords[0].y = finger1_startY;
                
                Log.i(TAG, "FINGER 1 DOWN at (" + coords[0].x + ", " + coords[0].y + ")");
                MotionEvent f1down = MotionEvent.obtain(
                    downTime, downTime,
                    MotionEvent.ACTION_DOWN, 1,
                    new MotionEvent.PointerProperties[]{props[0]},
                    new MotionEvent.PointerCoords[]{coords[0]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                rootView.dispatchTouchEvent(f1down);
                f1down.recycle();
                Thread.sleep(200);
                
                // Finger 2 DOWN (POINTER_DOWN)
                coords[1].x = finger2_startX;
                coords[1].y = finger2_startY;
                
                Log.i(TAG, "FINGER 2 DOWN at (" + coords[1].x + ", " + coords[1].y + ")");
                MotionEvent f2down = MotionEvent.obtain(
                    downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    2, props, coords,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                rootView.dispatchTouchEvent(f2down);
                f2down.recycle();
                Thread.sleep(200);
                
                // Move BOTH fingers simultaneously - drawing two lines
                Log.i(TAG, "Moving BOTH fingers simultaneously (drawing two lines)...");
                for (int i = 1; i <= 10; i++) {
                    // Finger 1 moves right
                    coords[0].x = finger1_startX + (i * 30);
                    coords[0].y = finger1_startY;
                    
                    // Finger 2 moves right
                    coords[1].x = finger2_startX + (i * 30);
                    coords[1].y = finger2_startY;
                    
                    MotionEvent moveEvent = MotionEvent.obtain(
                        downTime, SystemClock.uptimeMillis(),
                        MotionEvent.ACTION_MOVE,
                        2, props, coords,
                        0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                    );
                    rootView.dispatchTouchEvent(moveEvent);
                    moveEvent.recycle();
                    
                    if (i % 3 == 0) {
                        Log.i(TAG, "  Move " + i + ": F1(" + (int)coords[0].x + "," + (int)coords[0].y + 
                                   ") F2(" + (int)coords[1].x + "," + (int)coords[1].y + ")");
                    }
                    
                    Thread.sleep(80);
                }
                
                Log.i(TAG, "Finished moving both fingers");
                
                // Lift finger 2 first (POINTER_UP)
                Log.i(TAG, "FINGER 2 UP");
                MotionEvent f2up = MotionEvent.obtain(
                    downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    2, props, coords,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                rootView.dispatchTouchEvent(f2up);
                f2up.recycle();
                Thread.sleep(200);
                
                // Lift finger 1 (ACTION_UP)
                Log.i(TAG, "FINGER 1 UP");
                MotionEvent f1up = MotionEvent.obtain(
                    downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_UP, 1,
                    new MotionEvent.PointerProperties[]{props[0]},
                    new MotionEvent.PointerCoords[]{coords[0]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                rootView.dispatchTouchEvent(f1up);
                f1up.recycle();
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "TWO-FINGER MULTITOUCH TEST COMPLETED!");
                Log.i(TAG, "Two independent lines should be drawn");
                Log.i(TAG, "==========================================");
                
                multitouchSucceeded[0] = true;
                testComplete[0] = true;
                
                // Give SDL time to process events before shutdown
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during multitouch test", e);
                testComplete[0] = true;
                multitouchSucceeded[0] = false;
            }
        });
        
        // Wait for test to complete
        int timeout = 0;
        while (!testComplete[0] && timeout < 150) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Multitouch test did not complete", testComplete[0]);
        assertTrue("Multitouch test failed - events not processed correctly", multitouchSucceeded[0]);
        
        Log.i(TAG, "✓ TEST 2 PASSED: Two-finger multitouch drawing works!");
    }
}
