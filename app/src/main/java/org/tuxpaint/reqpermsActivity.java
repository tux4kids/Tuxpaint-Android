package org.tuxpaint;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.app.*;
import android.content.pm.PackageManager;


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

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S){
			this.requestPermissions(new String[]{ android.Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.BLUETOOTH_CONNECT }, 2);
		} else {
			this.requestPermissions(new String[]{ android.Manifest.permission.WRITE_EXTERNAL_STORAGE }, 2);
		}
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
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
					&& this.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
				finish();
			}
		}
    }

    protected void onDestroy() {
        Log.v(TAG, "onDestroy()");
        super.onDestroy();
    }
}
