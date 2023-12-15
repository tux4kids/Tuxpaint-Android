package org.tuxpaint;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.app.*;
import android.content.pm.PackageManager;
import android.widget.TextView;


/* reqpermsActivity */
/* Only gets called from the tuxpaintActivity after having checked the lack of permissions
   and the SDK_INT version > 22, so no more barrier checks here */ 
public class reqBTpermsActivity extends Activity {
    private static final String TAG = reqBTpermsActivity.class.getSimpleName();
    Button understoodButton = null;
TextView stateofBTpermsTextvieW = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
		Log.v(TAG, "onCreate()");
		super.onCreate(savedInstanceState);

			setContentView(R.layout.reqbtperms);

			stateofBTpermsTextvieW = (TextView) this.findViewById(R.id.stateofBTPerms);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			if (this.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED)
				stateofBTpermsTextvieW.setText(getResources().getString(R.string.BlueTooth_allowed));
			else
				stateofBTpermsTextvieW.setText(getResources().getString(R.string.BlueTooth_denied));
		}

		understoodButton = (Button) this.findViewById(R.id.buttonUnderstood);
				understoodButton.setOnClickListener(new View.OnClickListener() {
					public void onClick(View buttonView) {
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
							requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, 2);
						}
						finish();
					}});

				
	
	}

    protected void onDestroy() {
        Log.v(TAG, "onDestroy()");
        super.onDestroy();
    }
}
