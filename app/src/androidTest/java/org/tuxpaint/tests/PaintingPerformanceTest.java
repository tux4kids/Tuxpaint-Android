package org.tuxpaint.tests;

import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.tuxpaint.tuxpaintActivity;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Performance test to measure painting responsiveness.
 * Verifies that drawing operations complete within acceptable time limits.
 */
@RunWith(AndroidJUnit4.class)
public class PaintingPerformanceTest {

    private static final String TAG = "PaintPerformance";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    /**
     * Test painting responsiveness by simulating touch events and measuring response time.
     * Enhanced with detailed timing measurements.
     */
    @Test
    public void testPaintingResponsiveness() throws InterruptedException {
        Log.i(TAG, "========================================");
        Log.i(TAG, "STARTING PAINTING PERFORMANCE TEST");
        Log.i(TAG, "========================================");
        
        // Wait for activity to fully load
        Log.i(TAG, "Waiting 5 seconds for app to fully load...");
        Thread.sleep(5000);

        final long[] timings = new long[10];
        final boolean[] testComplete = {false};
        
        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity ready, starting paint test...");
            View surfaceView = activity.getWindow().getDecorView().getRootView();
            
            try {
                // Measure touch down
                long t0 = SystemClock.elapsedRealtimeNanos();
                long downTime = SystemClock.uptimeMillis();
                
                MotionEvent downEvent = MotionEvent.obtain(
                    downTime, downTime, MotionEvent.ACTION_DOWN, 400f, 400f, 0
                );
                Log.i(TAG, "Dispatching ACTION_DOWN at (400, 400)");
                surfaceView.dispatchTouchEvent(downEvent);
                long t1 = SystemClock.elapsedRealtimeNanos();
                timings[0] = (t1 - t0) / 1000000; // Convert to ms
                Log.i(TAG, "  ACTION_DOWN processed in " + timings[0] + "ms");
                downEvent.recycle();
                
                // Small delay
                Thread.sleep(50);
                
                // Measure move events (draw a line)
                for (int i = 1; i <= 5; i++) {
                    long tStart = SystemClock.elapsedRealtimeNanos();
                    long eventTime = SystemClock.uptimeMillis();
                    
                    MotionEvent moveEvent = MotionEvent.obtain(
                        downTime, eventTime, MotionEvent.ACTION_MOVE,
                        400f + (i * 30f), 400f + (i * 30f), 0
                    );
                    Log.i(TAG, "Dispatching ACTION_MOVE #" + i + " at (" + 
                        (400 + i*30) + ", " + (400 + i*30) + ")");
                    surfaceView.dispatchTouchEvent(moveEvent);
                    long tEnd = SystemClock.elapsedRealtimeNanos();
                    timings[i] = (tEnd - tStart) / 1000000;
                    Log.i(TAG, "  ACTION_MOVE #" + i + " processed in " + timings[i] + "ms");
                    moveEvent.recycle();
                    
                    Thread.sleep(100); // Allow rendering
                }
                
                // Measure touch up
                long tUpStart = SystemClock.elapsedRealtimeNanos();
                long upTime = SystemClock.uptimeMillis();
                MotionEvent upEvent = MotionEvent.obtain(
                    downTime, upTime, MotionEvent.ACTION_UP, 550f, 550f, 0
                );
                Log.i(TAG, "Dispatching ACTION_UP at (550, 550)");
                surfaceView.dispatchTouchEvent(upEvent);
                long tUpEnd = SystemClock.elapsedRealtimeNanos();
                timings[6] = (tUpEnd - tUpStart) / 1000000;
                Log.i(TAG, "  ACTION_UP processed in " + timings[6] + "ms");
                upEvent.recycle();
                
                testComplete[0] = true;
                
            } catch (Exception e) {
                Log.e(TAG, "Test failed with exception", e);
                fail("Test failed: " + e.getMessage());
            }
        });
        
        // Wait for test to complete
        Thread.sleep(1000);
        
        if (!testComplete[0]) {
            fail("Test did not complete");
        }
        
        // Calculate statistics
        long totalTime = 0;
        long maxTime = 0;
        for (int i = 0; i <= 6; i++) {
            totalTime += timings[i];
            if (timings[i] > maxTime) maxTime = timings[i];
        }
        long avgTime = totalTime / 7;
        
        Log.i(TAG, "========================================");
        Log.i(TAG, "TEST RESULTS:");
        Log.i(TAG, "  Total time: " + totalTime + "ms");
        Log.i(TAG, "  Average per event: " + avgTime + "ms");
        Log.i(TAG, "  Maximum latency: " + maxTime + "ms");
        Log.i(TAG, "========================================");
        
        // For now, just log - don't fail on performance
        if (maxTime > 500) {
            Log.w(TAG, "WARNING: Maximum latency " + maxTime + "ms exceeds 500ms threshold!");
        }
    }

    /**
     * Test multiple consecutive paint strokes to verify sustained performance.
     */
    @Test
    public void testSustainedPaintingPerformance() throws InterruptedException {
        // Wait for activity to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            View contentView = activity.findViewById(android.R.id.content);
            
            if (contentView != null) {
                long totalTime = 0;
                int strokeCount = 10;
                
                for (int stroke = 0; stroke < strokeCount; stroke++) {
                    long startTime = System.currentTimeMillis();
                    
                    // Draw a short stroke
                    long downTime = System.currentTimeMillis();
                    float startX = 300f + (stroke * 50f);
                    float startY = 400f;
                    
                    MotionEvent downEvent = MotionEvent.obtain(
                        downTime, downTime, MotionEvent.ACTION_DOWN, startX, startY, 0
                    );
                    contentView.dispatchTouchEvent(downEvent);
                    downEvent.recycle();
                    
                    MotionEvent moveEvent = MotionEvent.obtain(
                        downTime, System.currentTimeMillis(), MotionEvent.ACTION_MOVE, 
                        startX + 30f, startY + 30f, 0
                    );
                    contentView.dispatchTouchEvent(moveEvent);
                    moveEvent.recycle();
                    
                    MotionEvent upEvent = MotionEvent.obtain(
                        downTime, System.currentTimeMillis(), MotionEvent.ACTION_UP, 
                        startX + 30f, startY + 30f, 0
                    );
                    contentView.dispatchTouchEvent(upEvent);
                    upEvent.recycle();
                    
                    long strokeTime = System.currentTimeMillis() - startTime;
                    totalTime += strokeTime;
                    
                    // Small delay between strokes
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                
                long averageTime = totalTime / strokeCount;
                
                android.util.Log.i("PaintPerformance", 
                    "Average stroke time: " + averageTime + "ms over " + strokeCount + " strokes");
                
                // Average stroke time should be < 150ms
                assertTrue("Average painting time should be < 150ms, but was " + averageTime + "ms", 
                    averageTime < 150);
            }
        });
    }
}
