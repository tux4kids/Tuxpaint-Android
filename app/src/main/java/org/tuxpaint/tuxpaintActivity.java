package org.tuxpaint;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.widget.AbsoluteLayout;
import android.widget.Button;
import android.content.res.AssetManager;

public class tuxpaintActivity extends SDLActivity {
    private static final String TAG = "Tux Paint";
    private static View mConfigButton = null;

    private static AssetManager mgr;
    private static native boolean managertojni(AssetManager mgr);

    // Load the .so
    static {
        System.loadLibrary("stlport_shared");
        System.loadLibrary("tuxpaint_png");
        System.loadLibrary("tuxpaint_fribidi");
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
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("SDL2_ttf");
        System.loadLibrary("SDL2_Pango");
        System.loadLibrary("tuxpaint");
    }
 
    @Override
    protected void onCreate(Bundle savedInstanceState) {
	//	unzipAssets ();
    	super.onCreate(savedInstanceState);
	mgr = getResources().getAssets();
	managertojni(mgr);
    }

    /*
    private void unzipAssets(){
    	File internal = getFilesDir();
    	
    	// test whether tuxpaint.zip in assets folder is already unziped.
    	File brushes =  new File (internal, "data/brushes");
    	if (brushes.exists() && brushes.isDirectory())
    		return;

    	// unzip to /data/data/org.tuxpaint/files
	Log.d(TAG, "unzip tuxpaint.zip to /data/data/org.tuxpaint/files");
	     try {
	    	 InputStream in = getAssets().open("tuxpaint.zip");
	    	 ZipInputStream zin;
	         String name;
	         zin = new ZipInputStream(new BufferedInputStream(in));          
	         ZipEntry ze;
	         byte[] buffer = new byte[1024];
	         int count;
	         while ((ze = zin.getNextEntry()) != null) 
	         {
	        	 name = ze.getName();

	             if (ze.isDirectory()) {
	                File dir = new File(internal, name);
	                dir.mkdirs();
	                continue;
	             }

	             File file = new File (internal, name);
	             file.createNewFile();
	             FileOutputStream fout = new FileOutputStream(file);

	             while ((count = zin.read(buffer)) != -1) {
	                 fout.write(buffer, 0, count);             
	             }
	             fout.flush();
	             fout.close();               
	             zin.closeEntry();
	         }

	         zin.close();
	     } 
	     catch(IOException e) {
	         e.printStackTrace();
	     }
	     
    }
    */
    
    // private void addConfigButton () {
    // 	if (mConfigButton != null)
    // 		return;

    // 	DisplayMetrics dm = new DisplayMetrics();
    // 	getWindowManager().getDefaultDisplay().getMetrics(dm);
    // 	int width = dm.widthPixels;
    // 	int heigh = dm.heightPixels;
    // 	AbsoluteLayout.LayoutParams params = new AbsoluteLayout.LayoutParams
    //     		(60, 60, width-60, heigh-60);
    //     mConfigButton = new Button (getContext ());
    //     mConfigButton.setBackgroundResource(R.drawable.ic_settings_black_36dp);
    //     mConfigButton.setOnClickListener(new View.OnClickListener() {
    //   	  public void onClick(View v) {
    //       		startActivity(new Intent (tuxpaintActivity.this, ConfigActivity.class));
    //   	  }
    //     });
    //     SDLActivity.mLayout.addView(mConfigButton, params);
    // }
    
}
