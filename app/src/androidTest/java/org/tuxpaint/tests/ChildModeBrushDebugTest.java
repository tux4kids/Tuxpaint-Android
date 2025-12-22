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

/**
 * Debug test for child mode brush slider visibility issue.
 * 
 * User report: "die brushes werden immer noch nicht ausgeblendet, wenn man den child mode auswählt"
 */
@RunWith(AndroidJUnit4.class)
public class ChildModeBrushDebugTest {

    private static final String TAG = "ChildModeBrushDebug";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    @Test
    public void testChildModeActivationAndBrushVisibility() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "DEBUG: Child Mode Brush Visibility");
        Log.i(TAG, "==========================================");
        
        // Wait for app to load
        Thread.sleep(12000);

        activityRule.getScenario().onActivity(activity -> {
            Log.i(TAG, "Activity ready");
            View rootView = activity.getWindow().getDecorView().getRootView();
            
            try {
                int screenWidth = rootView.getWidth();
                int screenHeight = rootView.getHeight();
                
                Log.i(TAG, "Screen: " + screenWidth + "x" + screenHeight);
                
                // Try multiple positions for child mode button
                int[] xPositions = {95, 142, 190};
                int[] yPositions = {808, 850, 900, 950, 1000};
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "Trying to click child mode button...");
                Log.i(TAG, "Attempting different positions:");
                
                for (int x : xPositions) {
                    for (int y : yPositions) {
                        Log.i(TAG, "Trying position (" + x + ", " + y + ")");
                        clickAt(rootView, x, y);
                        Thread.sleep(500);
                    }
                }
                
                Thread.sleep(2000);
                
                Log.i(TAG, "==========================================");
                Log.i(TAG, "Check logcat for:");
                Log.i(TAG, "  - child_mode=1");
                Log.i(TAG, "  - 'Calling draw_child_mode_brush_slider()'");
                Log.i(TAG, "  - 'draw_brushes() called, child_mode=1'");
                Log.i(TAG, "==========================================");
                
            } catch (Exception e) {
                Log.e(TAG, "Error", e);
            }
        });
        
        Thread.sleep(5000);
    }
    
    private void clickAt(View rootView, int x, int y) throws InterruptedException {
        long downTime = SystemClock.uptimeMillis();
        
        MotionEvent downEvent = MotionEvent.obtain(
            downTime, downTime, 
            MotionEvent.ACTION_DOWN, 
            x, y, 0
        );
        rootView.dispatchTouchEvent(downEvent);
        downEvent.recycle();
        
        Thread.sleep(50);
        
        MotionEvent upEvent = MotionEvent.obtain(
            downTime, SystemClock.uptimeMillis(),
            MotionEvent.ACTION_UP,
            x, y, 0
        );
        rootView.dispatchTouchEvent(upEvent);
        upEvent.recycle();
        
        Thread.sleep(50);
    }
}
