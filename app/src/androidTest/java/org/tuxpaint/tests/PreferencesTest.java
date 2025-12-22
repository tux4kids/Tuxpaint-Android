package org.tuxpaint.tests;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.tuxpaint.tuxpaintActivity;

import static org.junit.Assert.*;

/**
 * Tests for the SharedPreferences save/load functionality.
 * Verifies that:
 * - savePreferences() correctly stores values
 * - loadPreferences() correctly retrieves values
 * - Default values are returned when no preferences exist
 */
@RunWith(AndroidJUnit4.class)
public class PreferencesTest {

    private static final String TAG = "PreferencesTest";
    private static final String PREFS_NAME = "TuxPaintPrefs";

    @Rule
    public ActivityScenarioRule<tuxpaintActivity> activityRule = 
        new ActivityScenarioRule<>(tuxpaintActivity.class);

    @Before
    public void setUp() {
        // Clear preferences before each test
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        prefs.edit().clear().commit();
        Log.i(TAG, "Cleared preferences before test");
    }

    /**
     * Test that savePreferences stores values correctly
     */
    @Test
    public void testSavePreferences() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: SAVE PREFERENCES");
        Log.i(TAG, "Testing preference save functionality");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            // Test data
            int useSound = 1;
            int childMode = 1;
            int childModeLocked = 0;
            int lastBrush = 5;
            int lastBrushCategory = 3;
            
            // Call savePreferences
            tuxpaintActivity.savePreferences(useSound, childMode, childModeLocked, 
                                            lastBrush, lastBrushCategory);
            
            // Verify values were saved
            SharedPreferences prefs = activity.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
            
            assertEquals("Sound preference not saved correctly", 
                        true, prefs.getBoolean("use_sound", false));
            assertEquals("Child mode preference not saved correctly", 
                        true, prefs.getBoolean("child_mode", false));
            assertEquals("Child mode locked preference not saved correctly", 
                        false, prefs.getBoolean("child_mode_locked", true));
            assertEquals("Last brush preference not saved correctly", 
                        5, prefs.getInt("last_brush", -1));
            assertEquals("Last brush category preference not saved correctly", 
                        3, prefs.getInt("last_brush_category", -1));
            
            Log.i(TAG, "✓ All preferences saved correctly");
        });
    }

    /**
     * Test that loadPreferences retrieves values correctly
     */
    @Test
    public void testLoadPreferences() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: LOAD PREFERENCES");
        Log.i(TAG, "Testing preference load functionality");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            // Set up test data in SharedPreferences
            SharedPreferences prefs = activity.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean("use_sound", false);
            editor.putBoolean("child_mode", true);
            editor.putBoolean("child_mode_locked", true);
            editor.putInt("last_brush", 7);
            editor.putInt("last_brush_category", 4);
            editor.commit();
            
            // Call loadPreferences
            int[] values = tuxpaintActivity.loadPreferences();
            
            // Verify values were loaded correctly
            assertNotNull("loadPreferences returned null", values);
            assertEquals("Array length incorrect", 5, values.length);
            assertEquals("Sound preference not loaded correctly", 0, values[0]);
            assertEquals("Child mode preference not loaded correctly", 1, values[1]);
            assertEquals("Child mode locked preference not loaded correctly", 1, values[2]);
            assertEquals("Last brush preference not loaded correctly", 7, values[3]);
            assertEquals("Last brush category preference not loaded correctly", 4, values[4]);
            
            Log.i(TAG, "✓ All preferences loaded correctly");
        });
    }

    /**
     * Test that loadPreferences returns defaults when no preferences exist
     */
    @Test
    public void testLoadPreferencesDefaults() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: LOAD PREFERENCES DEFAULTS");
        Log.i(TAG, "Testing default values when no preferences exist");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            // Preferences were cleared in setUp()
            // Call loadPreferences
            int[] values = tuxpaintActivity.loadPreferences();
            
            // Verify default values
            assertNotNull("loadPreferences returned null", values);
            assertEquals("Array length incorrect", 5, values.length);
            assertEquals("Default sound preference incorrect", 1, values[0]); // Sound on by default
            assertEquals("Default child mode preference incorrect", 0, values[1]); // Child mode off by default
            assertEquals("Default child mode locked preference incorrect", 0, values[2]); // Unlocked by default
            assertEquals("Default last brush preference incorrect", 0, values[3]); // Brush 0 by default
            assertEquals("Default last brush category preference incorrect", 0, values[4]); // Category 0 by default
            
            Log.i(TAG, "✓ All default values correct");
        });
    }

    /**
     * Test save and load round-trip with multiple value combinations
     */
    @Test
    public void testSaveLoadRoundTrip() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: SAVE/LOAD ROUND TRIP");
        Log.i(TAG, "Testing multiple save/load cycles");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            // Test multiple combinations
            int[][] testCases = {
                {0, 0, 0, 0, 0},  // All off/zero
                {1, 1, 1, 10, 5}, // All on/non-zero
                {1, 0, 1, 3, 2},  // Mixed values
                {0, 1, 0, 15, 8}  // Mixed values 2
            };
            
            for (int i = 0; i < testCases.length; i++) {
                int[] testCase = testCases[i];
                
                // Save
                tuxpaintActivity.savePreferences(testCase[0], testCase[1], testCase[2], 
                                                testCase[3], testCase[4]);
                
                // Load
                int[] loaded = tuxpaintActivity.loadPreferences();
                
                // Verify
                assertArrayEquals("Round trip failed for test case " + i, testCase, loaded);
                Log.i(TAG, "✓ Round trip " + (i+1) + " successful: [" + 
                     testCase[0] + "," + testCase[1] + "," + testCase[2] + "," + 
                     testCase[3] + "," + testCase[4] + "]");
            }
            
            Log.i(TAG, "✓ All round trips successful");
        });
    }

    /**
     * Test that boolean to int conversion works correctly
     */
    @Test
    public void testBooleanIntConversion() throws InterruptedException {
        Log.i(TAG, "==========================================");
        Log.i(TAG, "TEST: BOOLEAN/INT CONVERSION");
        Log.i(TAG, "Testing boolean storage and int retrieval");
        Log.i(TAG, "==========================================");
        
        // Wait for app to fully load
        Thread.sleep(3000);

        activityRule.getScenario().onActivity(activity -> {
            // Save with non-zero values (should be true)
            tuxpaintActivity.savePreferences(5, 10, 100, 0, 0);
            
            // Load and verify they come back as 1 (true)
            int[] values = tuxpaintActivity.loadPreferences();
            assertEquals("Non-zero should convert to 1", 1, values[0]);
            assertEquals("Non-zero should convert to 1", 1, values[1]);
            assertEquals("Non-zero should convert to 1", 1, values[2]);
            
            Log.i(TAG, "✓ Boolean conversion works correctly");
        });
    }
}
