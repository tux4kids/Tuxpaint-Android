package org.tuxpaint;

import java.io.IOException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.content.res.AssetManager;
import android.content.pm.PackageManager;
import android.view.MotionEvent;
import android.widget.Toast;
import android.Manifest;

public class tuxpaintActivity extends SDLActivity {
    private static final String TAG = "Tux Paint";
    private static final String PREFS_NAME = "TuxPaintPrefs";
    private static AssetManager mgr;
    private static tuxpaintActivity instance;
    
    private static native boolean managertojni(AssetManager mgr);
    private static native void setnativelibdir(String path);
    private static native void setfilesdir(String path);
    private static native void nativeOnTouch(int action, int pointerCount, float[] x, float[] y, long[] pointerIds);
    
    // Called by SDLSurface via reflection to forward all multitouch data
    public static void forwardMultitouchToNative(int action, int pointerCount, float[] x, float[] y, long[] pointerIds) {
        nativeOnTouch(action, pointerCount, x, y, pointerIds);
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate()");
        instance = this;

        boolean requestPermissions = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Build.VERSION.SDK_INT <= Build.VERSION_CODES.R) {
            if (this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                Intent intent = new Intent(this, reqpermsActivity.class);
		this.startActivity(intent);
            }
        }

        mgr = getResources().getAssets();
        
        // Generate stamp manifest at runtime if needed
        try {
            generateStampManifest();
        } catch (IOException e) {
            Log.e(TAG, "Failed to generate stamp manifest", e);
        }

        super.onCreate(savedInstanceState);
        managertojni(mgr);
        setnativelibdir(getApplicationInfo().nativeLibraryDir + "/");
        setfilesdir(getFilesDir().getAbsolutePath());
    }
    
    @Override
    protected void onDestroy() {
        instance = null;
        super.onDestroy();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // Forward multitouch data to native code
        int action = event.getActionMasked();
        int pointerCount = event.getPointerCount();
        
        if (pointerCount > 0) {
            float[] x = new float[pointerCount];
            float[] y = new float[pointerCount];
            long[] pointerIds = new long[pointerCount];
            
            for (int i = 0; i < pointerCount; i++) {
                x[i] = event.getX(i);
                y[i] = event.getY(i);
                pointerIds[i] = event.getPointerId(i);
            }
            
            nativeOnTouch(action, pointerCount, x, y, pointerIds);
        }
        
        // Let SDL handle it too
        return super.onTouchEvent(event);
    }
    
    /**
     * Save child mode preferences to SharedPreferences
     * Called from native code via JNI
     * @param useSound Sound toggle status (1 = enabled, 0 = disabled)
     * @param childMode Child mode status (1 = enabled, 0 = disabled)
     * @param childModeLocked Child mode locked status (1 = locked, 0 = unlocked)
     * @param lastBrush Last brush index in child mode
     * @param lastBrushCategory Last brush category in child mode
     */
    public static void savePreferences(int useSound, int childMode, int childModeLocked, int lastBrush, int lastBrushCategory) {
        if (instance == null) {
            Log.w(TAG, "savePreferences() called but instance is null");
            return;
        }
        
        SharedPreferences prefs = instance.getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        
        editor.putBoolean("use_sound", useSound != 0);
        editor.putBoolean("child_mode", childMode != 0);
        editor.putBoolean("child_mode_locked", childModeLocked != 0);
        editor.putInt("last_brush", lastBrush);
        editor.putInt("last_brush_category", lastBrushCategory);
        
        editor.apply();
        
        Log.d(TAG, "Saved preferences: sound=" + useSound + " childMode=" + childMode + 
              " locked=" + childModeLocked + " brush=" + lastBrush + " category=" + lastBrushCategory);
    }
    
    /**
     * Load child mode preferences from SharedPreferences
     * Called from native code via JNI
     * @return Array of 5 integers: [useSound, childMode, childModeLocked, lastBrush, lastBrushCategory]
     */
    public static int[] loadPreferences() {
        if (instance == null) {
            Log.w(TAG, "loadPreferences() called but instance is null, returning defaults");
            return new int[] {1, 0, 0, 0, 0};  // Default: sound on, child mode off, unlocked, brush 0, category 0
        }
        
        SharedPreferences prefs = instance.getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        
        // Check if this is first launch (no preferences saved yet)
        boolean isFirstLaunch = !prefs.contains("child_mode");
        
        if (isFirstLaunch) {
            Log.i(TAG, "First launch detected - Setting defaults: Kids Mode, Category 3, Locked");
            
            // First launch: Start in Kids Mode with category 3, locked
            int useSound = 1;           // Sound enabled
            int childMode = 1;          // Kids Mode enabled
            int childModeLocked = 1;    // Locked
            int lastBrush = 36;          // First brush
            int lastBrushCategory = 2;  // Category 3
            
            // Save these defaults
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean("use_sound", true);
            editor.putBoolean("child_mode", true);
            editor.putBoolean("child_mode_locked", true);
            editor.putInt("last_brush", lastBrush);
            editor.putInt("last_brush_category", lastBrushCategory);
            editor.apply();
            
            // Show info message about unlocking (run on UI thread)
            instance.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(instance, 
                        "Kids Mode is active and locked.",
                        Toast.LENGTH_LONG).show();
                }
            });
            
            // Show unlock instructions after delay
            instance.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    final android.os.Handler handler = new android.os.Handler();
                    handler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(instance, 
                                "To unlock: Press and hold the Kids Button for 3 seconds.",
                                Toast.LENGTH_LONG).show();
                        }
                    }, 2500);
                }
            });
            
            Log.d(TAG, "First launch defaults saved and info message shown");
            return new int[] {useSound, childMode, childModeLocked, lastBrush, lastBrushCategory};
        }
        
        // Normal load from existing preferences
        int useSound = prefs.getBoolean("use_sound", true) ? 1 : 0;
        int childMode = prefs.getBoolean("child_mode", false) ? 1 : 0;
        int childModeLocked = prefs.getBoolean("child_mode_locked", false) ? 1 : 0;
        int lastBrush = prefs.getInt("last_brush", 0);
        int lastBrushCategory = prefs.getInt("last_brush_category", 0);
        
        Log.d(TAG, "Loaded preferences: sound=" + useSound + " childMode=" + childMode + 
              " locked=" + childModeLocked + " brush=" + lastBrush + " category=" + lastBrushCategory);
        
        // Show reminder if locked
        if (childModeLocked != 0) {
            instance.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(instance, 
                        "Kids Mode is locked.",
                        Toast.LENGTH_SHORT).show();
                }
            });
            
            // Show unlock instructions after delay
            instance.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    final android.os.Handler handler = new android.os.Handler();
                    handler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(instance, 
                                "To unlock: Hold tool for 3 seconds.",
                                Toast.LENGTH_LONG).show();
                        }
                    }, 2000);
                }
            });
        }
        
        return new int[] {useSound, childMode, childModeLocked, lastBrush, lastBrushCategory};
    }
    
    /**
     * Generate stamp manifest at runtime by scanning assets/stamps directory
     * Manifest includes version number and is regenerated if version changes
     */
    private void generateStampManifest() throws IOException {
        String manifestPath = getFilesDir() + "/stamp_manifest";
        File manifestFile = new File(manifestPath);
        
        Log.d(TAG, "Manifest path: " + manifestPath);
        Log.d(TAG, "Manifest exists: " + manifestFile.exists());
        
        // Get current app version
        int currentVersion = 0;
        try {
            currentVersion = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Could not get version code", e);
        }
        
        // Check if manifest exists and is up to date
        boolean needsRegeneration = !manifestFile.exists();
        
        if (manifestFile.exists()) {
            // Read first line to check version
            try {
                Log.d(TAG, "Reading manifest from: " + manifestFile.getAbsolutePath());
                Log.d(TAG, "File size: " + manifestFile.length() + " bytes");
                
                java.io.BufferedReader reader = new java.io.BufferedReader(new java.io.FileReader(manifestFile));
                String firstLine = reader.readLine();
                reader.close();
                
                Log.d(TAG, "First line read: '" + firstLine + "' (length=" + (firstLine != null ? firstLine.length() : 0) + ")");
                
                if (firstLine != null && firstLine.startsWith("# VERSION=")) {
                    String versionStr = firstLine.substring(10).trim(); // "# VERSION=" is 10 chars
                    Log.d(TAG, "Version string extracted: '" + versionStr + "'");
                    int savedVersion = Integer.parseInt(versionStr);
                    Log.d(TAG, "Manifest version: " + savedVersion + ", current version: " + currentVersion);
                    if (savedVersion != currentVersion) {
                        Log.i(TAG, "App updated: " + savedVersion + " -> " + currentVersion + ", regenerating manifest");
                        needsRegeneration = true;
                    } else {
                        Log.d(TAG, "Manifest is current, reusing");
                    }
                } else {
                    Log.w(TAG, "Manifest has no valid version header, regenerating");
                    needsRegeneration = true;
                }
            } catch (Exception e) {
                Log.w(TAG, "Error reading manifest version", e);
                needsRegeneration = true;
            }
        } else {
            Log.i(TAG, "Manifest does not exist, creating new one");
        }
        
        if (!needsRegeneration) {
            Log.d(TAG, "Stamp manifest already exists and is current");
            return;
        }
        
        // Delete old manifest to ensure clean write
        if (manifestFile.exists()) {
            manifestFile.delete();
            Log.d(TAG, "Deleted old manifest file");
        }
        
        Log.i(TAG, "Generating stamp manifest at runtime...");
        
        FileOutputStream fos = new FileOutputStream(manifestFile);
        OutputStreamWriter writer = new OutputStreamWriter(fos);
        
        // Write version header
        writer.write("# VERSION=" + currentVersion + "\n");
        
        // Scan stamps directory (in data/stamps for consistency with other assets)
        String[] categories = mgr.list("data/stamps");
        if (categories != null) {
            Arrays.sort(categories);
            
            for (String category : categories) {
                // Skip hidden files
                if (category.startsWith(".")) continue;
                
                writer.write("[" + category + "]\n");
                
                // Recursively find all subdirectories with stamps
                ArrayList<String> dirs = new ArrayList<>();
                scanStampDirectory("data/stamps/" + category, category, dirs);
                
                Collections.sort(dirs);
                for (String dir : dirs) {
                    writer.write(dir + "\n");
                }
                
                writer.write("\n");
                
                Log.d(TAG, "Category '" + category + "': " + dirs.size() + " directories");
            }
        }
        
        writer.flush();
        fos.flush();
        writer.close();
        fos.close();
        
        Log.i(TAG, "Stamp manifest generated successfully at " + manifestPath);
        
        // Verify the file was written correctly
        if (manifestFile.exists()) {
            try {
                java.io.BufferedReader verifyReader = new java.io.BufferedReader(new java.io.FileReader(manifestFile));
                String verifyLine = verifyReader.readLine();
                verifyReader.close();
                Log.d(TAG, "Verification: First line of new manifest: " + verifyLine);
            } catch (Exception e) {
                Log.w(TAG, "Could not verify manifest", e);
            }
        }
    }
    
    /**
     * Recursively scan directory for subdirectories containing stamp files
     */
    private void scanStampDirectory(String path, String relativePath, ArrayList<String> result) throws IOException {
        String[] entries = mgr.list(path);
        if (entries == null || entries.length == 0) return;
        
        boolean hasStamps = false;
        ArrayList<String> subdirs = new ArrayList<>();
        
        for (String entry : entries) {
            // Check if it's a file (has extension)
            if (entry.endsWith(".png") || entry.endsWith(".svg")) {
                hasStamps = true;
            } else if (!entry.contains(".")) {
                // Likely a subdirectory
                subdirs.add(entry);
            }
        }
        
        // Add this directory if it contains stamps
        if (hasStamps) {
            result.add(relativePath);
        }
        
        // Recurse into subdirectories
        for (String subdir : subdirs) {
            scanStampDirectory(path + "/" + subdir, relativePath + "/" + subdir, result);
        }
    }

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tuxpaint_png");
        System.loadLibrary("tuxpaint_fribidi");
        System.loadLibrary("SDL2");
        System.loadLibrary("tp_android_assets_fopen");
        System.loadLibrary("tuxpaint_intl");
        System.loadLibrary("tuxpaint_iconv");
        System.loadLibrary("tuxpaint_pixman");
        System.loadLibrary("tuxpaint_xml2");
        System.loadLibrary("tuxpaint_freetype");
        System.loadLibrary("tuxpaint_fontconfig");
        System.loadLibrary("tuxpaint_ffi");
        System.loadLibrary("tuxpaint_glib");
        System.loadLibrary("tuxpaint_cairo");
        System.loadLibrary("tuxpaint_harfbuzz_ng");
        System.loadLibrary("tuxpaint_pango");
        System.loadLibrary("tuxpaint_gdk_pixbuf");
        System.loadLibrary("tuxpaint_croco");
	//        System.loadLibrary("tuxpaint_rsvg");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_Pango");
	System.loadLibrary("SDL2_gfx");
        System.loadLibrary("tuxpaint");
    }
}
