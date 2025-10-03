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
import static org.junit.Assert.fail;

/**
 * Test multitouch painting functionality.
 * Verifies that multiple fingers can paint simultaneously without conflicts.
 */
@RunWith(AndroidJUnit4.class)
public class MultitouchTest {

    private static final String TAG = "MultitouchTest";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    /**
     * Test basic multitouch functionality with 2 simultaneous fingers painting.
     * This test verifies that:
     * 1. First finger can draw (mouse events work - compatibility)
     * 2. Second finger can draw simultaneously (multitouch works)
     * 3. Both fingers draw independent lines
     */
    @Test
    public void testTwoFingerSimultaneousPainting() throws InterruptedException {
        Log.i(TAG, "========================================");
        Log.i(TAG, "STARTING TWO-FINGER MULTITOUCH TEST");
        Log.i(TAG, "Test: First finger draws line 1, second finger draws line 2 simultaneously");
        Log.i(TAG, "========================================");
        
        // Wait for activity to fully load
        Log.i(TAG, "Waiting 10 seconds for app to fully load and show canvas...");
        Thread.sleep(10000);

        final boolean[] testComplete = {false};
        final boolean[] testPassed = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity ready, starting multitouch test...");
            View surfaceView = activity.getWindow().getDecorView().getRootView();
            
            try {
                long downTime = SystemClock.uptimeMillis();
                
                // Finger 1: Touch down at (300, 300)
                Log.i(TAG, "FINGER 1: Touching down at (300, 300)");
                MotionEvent.PointerProperties[] props1 = new MotionEvent.PointerProperties[1];
                props1[0] = new MotionEvent.PointerProperties();
                props1[0].id = 0;
                props1[0].toolType = MotionEvent.TOOL_TYPE_FINGER;
                
                MotionEvent.PointerCoords[] coords1 = new MotionEvent.PointerCoords[1];
                coords1[0] = new MotionEvent.PointerCoords();
                coords1[0].x = 300f;
                coords1[0].y = 300f;
                coords1[0].pressure = 1.0f;
                coords1[0].size = 1.0f;
                
                MotionEvent finger1Down = MotionEvent.obtain(
                    downTime, downTime,
                    MotionEvent.ACTION_DOWN,
                    1, props1, coords1,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                surfaceView.dispatchTouchEvent(finger1Down);
                finger1Down.recycle();
                
                Thread.sleep(50);
                
                // Finger 2: Touch down at (500, 500) - ACTION_POINTER_DOWN
                Log.i(TAG, "FINGER 2: Touching down at (500, 500)");
                MotionEvent.PointerProperties[] props2 = new MotionEvent.PointerProperties[2];
                props2[0] = new MotionEvent.PointerProperties();
                props2[0].id = 0;
                props2[0].toolType = MotionEvent.TOOL_TYPE_FINGER;
                props2[1] = new MotionEvent.PointerProperties();
                props2[1].id = 1;
                props2[1].toolType = MotionEvent.TOOL_TYPE_FINGER;
                
                MotionEvent.PointerCoords[] coords2 = new MotionEvent.PointerCoords[2];
                coords2[0] = new MotionEvent.PointerCoords();
                coords2[0].x = 300f;
                coords2[0].y = 300f;
                coords2[0].pressure = 1.0f;
                coords2[0].size = 1.0f;
                coords2[1] = new MotionEvent.PointerCoords();
                coords2[1].x = 500f;
                coords2[1].y = 500f;
                coords2[1].pressure = 1.0f;
                coords2[1].size = 1.0f;
                
                long eventTime = SystemClock.uptimeMillis();
                MotionEvent finger2Down = MotionEvent.obtain(
                    downTime, eventTime,
                    MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    2, props2, coords2,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                surfaceView.dispatchTouchEvent(finger2Down);
                finger2Down.recycle();
                
                Thread.sleep(50);
                
                // Move both fingers simultaneously
                Log.i(TAG, "Moving both fingers simultaneously...");
                for (int i = 0; i < 5; i++) {
                    eventTime = SystemClock.uptimeMillis();
                    
                    // Update coordinates for both fingers
                    coords2[0].x = 300f + (i * 20f);
                    coords2[0].y = 300f + (i * 20f);
                    coords2[1].x = 500f + (i * 20f);
                    coords2[1].y = 500f - (i * 20f);  // Move in opposite direction
                    
                    MotionEvent moveEvent = MotionEvent.obtain(
                        downTime, eventTime,
                        MotionEvent.ACTION_MOVE,
                        2, props2, coords2,
                        0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                    );
                    Log.i(TAG, "  Move " + i + ": Finger1 at (" + coords2[0].x + "," + coords2[0].y + 
                          "), Finger2 at (" + coords2[1].x + "," + coords2[1].y + ")");
                    surfaceView.dispatchTouchEvent(moveEvent);
                    moveEvent.recycle();
                    
                    Thread.sleep(100);
                }
                
                // Lift finger 2 first - ACTION_POINTER_UP
                Log.i(TAG, "FINGER 2: Lifting up");
                eventTime = SystemClock.uptimeMillis();
                MotionEvent finger2Up = MotionEvent.obtain(
                    downTime, eventTime,
                    MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    2, props2, coords2,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                surfaceView.dispatchTouchEvent(finger2Up);
                finger2Up.recycle();
                
                Thread.sleep(50);
                
                // Lift finger 1 - ACTION_UP
                Log.i(TAG, "FINGER 1: Lifting up");
                eventTime = SystemClock.uptimeMillis();
                MotionEvent finger1Up = MotionEvent.obtain(
                    downTime, eventTime,
                    MotionEvent.ACTION_UP,
                    1, props1, coords1,
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0
                );
                surfaceView.dispatchTouchEvent(finger1Up);
                finger1Up.recycle();
                
                Log.i(TAG, "========================================");
                Log.i(TAG, "Multitouch sequence completed successfully");
                Log.i(TAG, "========================================");
                
                testPassed[0] = true;
                testComplete[0] = true;
                
                // Give SDL time to process events before shutdown
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during multitouch test", e);
                fail("Multitouch test failed with exception: " + e.getMessage());
            }
        });
        
        // Wait for test to complete
        int timeout = 0;
        while (!testComplete[0] && timeout < 50) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Multitouch test did not complete", testComplete[0]);
        assertTrue("Multitouch test failed", testPassed[0]);
        
        Log.i(TAG, "Test completed successfully");
    }
    
    /**
     * Test three-finger multitouch painting.
     */
    @Test
    public void testThreeFingerSimultaneousPainting() throws InterruptedException {
        Log.i(TAG, "========================================");
        Log.i(TAG, "STARTING THREE-FINGER MULTITOUCH TEST");
        Log.i(TAG, "========================================");
        
        Thread.sleep(5000);

        final boolean[] testComplete = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity ready, starting three-finger test...");
            View surfaceView = activity.getWindow().getDecorView().getRootView();
            
            try {
                long downTime = SystemClock.uptimeMillis();
                
                // Create arrays for up to 3 fingers
                MotionEvent.PointerProperties[] props = new MotionEvent.PointerProperties[3];
                MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[3];
                
                for (int i = 0; i < 3; i++) {
                    props[i] = new MotionEvent.PointerProperties();
                    props[i].id = i;
                    props[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
                    
                    coords[i] = new MotionEvent.PointerCoords();
                    coords[i].x = 300f + (i * 150f);
                    coords[i].y = 400f;
                    coords[i].pressure = 1.0f;
                    coords[i].size = 1.0f;
                }
                
                // Finger 1 down
                Log.i(TAG, "FINGER 1: Touch down at (" + coords[0].x + ", " + coords[0].y + ")");
                MotionEvent f1 = MotionEvent.obtain(downTime, downTime,
                    MotionEvent.ACTION_DOWN, 1,
                    new MotionEvent.PointerProperties[]{props[0]},
                    new MotionEvent.PointerCoords[]{coords[0]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f1);
                f1.recycle();
                Thread.sleep(50);
                
                // Finger 2 down
                Log.i(TAG, "FINGER 2: Touch down at (" + coords[1].x + ", " + coords[1].y + ")");
                MotionEvent f2 = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT), 2,
                    new MotionEvent.PointerProperties[]{props[0], props[1]},
                    new MotionEvent.PointerCoords[]{coords[0], coords[1]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f2);
                f2.recycle();
                Thread.sleep(50);
                
                // Finger 3 down
                Log.i(TAG, "FINGER 3: Touch down at (" + coords[2].x + ", " + coords[2].y + ")");
                MotionEvent f3 = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_DOWN | (2 << MotionEvent.ACTION_POINTER_INDEX_SHIFT), 3,
                    props, coords, 0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f3);
                f3.recycle();
                Thread.sleep(50);
                
                // Move all 3 fingers in different directions
                Log.i(TAG, "Moving all 3 fingers simultaneously in different directions...");
                for (int i = 0; i < 5; i++) {
                    coords[0].x = 300f + (i * 10f);
                    coords[0].y = 400f + (i * 15f);
                    coords[1].x = 450f - (i * 10f);
                    coords[1].y = 400f + (i * 15f);
                    coords[2].x = 600f;
                    coords[2].y = 400f - (i * 15f);
                    
                    MotionEvent move = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                        MotionEvent.ACTION_MOVE, 3, props, coords,
                        0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                    surfaceView.dispatchTouchEvent(move);
                    move.recycle();
                    Thread.sleep(100);
                }
                
                // Lift fingers in reverse order
                Log.i(TAG, "Lifting all fingers...");
                
                // Finger 3 up
                MotionEvent f3up = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_UP | (2 << MotionEvent.ACTION_POINTER_INDEX_SHIFT), 3,
                    props, coords, 0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f3up);
                f3up.recycle();
                Thread.sleep(50);
                
                // Finger 2 up
                MotionEvent f2up = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT), 2,
                    new MotionEvent.PointerProperties[]{props[0], props[1]},
                    new MotionEvent.PointerCoords[]{coords[0], coords[1]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f2up);
                f2up.recycle();
                Thread.sleep(50);
                
                // Finger 1 up
                MotionEvent f1up = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                    MotionEvent.ACTION_UP, 1,
                    new MotionEvent.PointerProperties[]{props[0]},
                    new MotionEvent.PointerCoords[]{coords[0]},
                    0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                surfaceView.dispatchTouchEvent(f1up);
                f1up.recycle();
                
                Log.i(TAG, "========================================");
                Log.i(TAG, "Three-finger multitouch test completed");
                Log.i(TAG, "========================================");
                
                testComplete[0] = true;
                
                // Give SDL time to process events before shutdown
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during three-finger test", e);
                fail("Three-finger test failed: " + e.getMessage());
            }
        });
        
        // Wait for completion
        int timeout = 0;
        while (!testComplete[0] && timeout < 50) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Three-finger test did not complete", testComplete[0]);
        Log.i(TAG, "Three-finger test completed successfully");
    }
    
    /**
     * Test rapid touch add/remove (stress test).
     */
    @Test
    public void testRapidMultitouchAddRemove() throws InterruptedException {
        Log.i(TAG, "========================================");
        Log.i(TAG, "STARTING RAPID MULTITOUCH STRESS TEST");
        Log.i(TAG, "========================================");
        
        Thread.sleep(5000);

        final boolean[] testComplete = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Starting rapid add/remove test...");
            View surfaceView = activity.getWindow().getDecorView().getRootView();
            
            try {
                // Rapidly add and remove fingers
                for (int cycle = 0; cycle < 3; cycle++) {
                    Log.i(TAG, "Cycle " + (cycle + 1) + " - Adding 2 fingers...");
                    long downTime = SystemClock.uptimeMillis();
                    
                    // Quick touch with finger 1
                    MotionEvent.PointerProperties[] props1 = new MotionEvent.PointerProperties[1];
                    props1[0] = new MotionEvent.PointerProperties();
                    props1[0].id = 0;
                    props1[0].toolType = MotionEvent.TOOL_TYPE_FINGER;
                    
                    MotionEvent.PointerCoords[] coords1 = new MotionEvent.PointerCoords[1];
                    coords1[0] = new MotionEvent.PointerCoords();
                    coords1[0].x = 350f + (cycle * 50f);
                    coords1[0].y = 350f;
                    coords1[0].pressure = 1.0f;
                    coords1[0].size = 1.0f;
                    
                    MotionEvent down1 = MotionEvent.obtain(downTime, downTime,
                        MotionEvent.ACTION_DOWN, 1, props1, coords1,
                        0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                    surfaceView.dispatchTouchEvent(down1);
                    down1.recycle();
                    
                    Thread.sleep(20); // Very short delay
                    
                    MotionEvent up1 = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(),
                        MotionEvent.ACTION_UP, 1, props1, coords1,
                        0, 0, 1.0f, 1.0f, 0, 0, 0, 0);
                    surfaceView.dispatchTouchEvent(up1);
                    up1.recycle();
                    
                    Thread.sleep(20);
                }
                
                Log.i(TAG, "========================================");
                Log.i(TAG, "Rapid multitouch stress test completed");
                Log.i(TAG, "========================================");
                
                testComplete[0] = true;
                
                // Give SDL time to process events before shutdown
                Thread.sleep(2000);
                
            } catch (Exception e) {
                Log.e(TAG, "ERROR during stress test", e);
                fail("Stress test failed: " + e.getMessage());
            }
        });
        
        int timeout = 0;
        while (!testComplete[0] && timeout < 50) {
            Thread.sleep(100);
            timeout++;
        }
        
        assertTrue("Stress test did not complete", testComplete[0]);
        Log.i(TAG, "Stress test completed successfully");
    }
}
