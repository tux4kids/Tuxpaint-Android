package org.tuxpaint;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.app.*;
import android.content.pm.PackageManager;
import android.view.WindowManager.LayoutParams;
import android.view.WindowManager;


/* reqpermsActivity */
/* Only gets called from the tuxpaintActivity after having checked the lack of permissions
   and the SDK_INT version > 22, so no more barrier checks here */ 
public class reqpermsActivity extends Activity {
    private static final String TAG = reqpermsActivity.class.getSimpleName();
    Button understoodButton = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
	Log.v(TAG, "onCreate()");
	super.onCreate(savedInstanceState);

	this.requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 2);
	setContentView(R.layout.reqperms);

	understoodButton = (Button)this.findViewById(R.id.buttonUnderstood);

	understoodButton.setOnClickListener(new View.OnClickListener() {
		public void onClick(View buttonView) {
		    finish();
		}
	    });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
					   String[] permissions, int[] grantResults) {
	if (this.checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
	    finish();
	}
    }

    protected void onDestroy() {
        Log.v(TAG, "onDestroy()");
        super.onDestroy();
    }
}
