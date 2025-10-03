package org.tuxpaint;

import java.io.IOException;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.content.res.AssetManager;
import android.content.pm.PackageManager;
import android.view.MotionEvent;
import android.Manifest;

public class tuxpaintActivity extends SDLActivity {
    private static final String TAG = "Tux Paint";
    private static AssetManager mgr;
    private static native boolean managertojni(AssetManager mgr);
    private static native void setnativelibdir(String path);
    private static native void nativeOnTouch(int action, int pointerCount, float[] x, float[] y, long[] pointerIds);
    
    // Called by SDLSurface via reflection to forward all multitouch data
    public static void forwardMultitouchToNative(int action, int pointerCount, float[] x, float[] y, long[] pointerIds) {
        nativeOnTouch(action, pointerCount, x, y, pointerIds);
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate()");

        boolean requestPermissions = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Build.VERSION.SDK_INT <= Build.VERSION_CODES.R) {
            if (this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                Intent intent = new Intent(this, reqpermsActivity.class);
		this.startActivity(intent);
            }
        }


        super.onCreate(savedInstanceState);
        mgr = getResources().getAssets();
        managertojni(mgr);
        setnativelibdir(getApplicationInfo().nativeLibraryDir + "/");
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
