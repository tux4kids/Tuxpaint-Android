package org.tuxpaint;

import java.io.IOException;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.content.res.AssetManager;
import android.content.pm.PackageManager;
import android.Manifest;

public class tuxpaintActivity extends SDLActivity {
    private static final String TAG = "Tux Paint";
    private static AssetManager mgr;
    private static native boolean managertojni(AssetManager mgr);


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate()");

        if (android.os.Build.VERSION.SDK_INT > 22) {
            if (this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                Intent intent = new Intent(this, reqpermsActivity.class);
                this.startActivity(intent);
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (this.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                Intent intent = new Intent(this, reqpermsActivity.class);
                this.startActivity(intent);
            }
        }

        super.onCreate(savedInstanceState);
        mgr = getResources().getAssets();
        managertojni(mgr);
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
        System.loadLibrary("tuxpaint_rsvg");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_Pango");
	System.loadLibrary("SDL2_gfx");
        System.loadLibrary("tuxpaint");
    }
}
