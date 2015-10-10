package org.tuxpaint;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;
import java.util.Properties;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.ToggleButton;

public class ConfigActivity extends Activity {
	private static final String TAG = ConfigActivity.class.getSimpleName();
   	String[] locales = null;
	private Properties props = null;
	// current the configurable properties
	String autosave = null;
	String sound = null;
	String locale = null;
	String savedir = null;
	String datadir = null;
	String saveover = null;
	String startblank = null;
	
	EditText savedirView = null;
	EditText datadirView = null;
	ToggleButton soundToggle = null; 
	ToggleButton autosaveToggle = null;
	ToggleButton startblankToggle = null;
	Spinner localeSpinner = null;
	RadioGroup saveoverGroup = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate()");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.config);
		load ();
		
		savedirView = (EditText)this.findViewById(R.id.textSavedir);
		savedirView.setText(savedir);
		savedirView.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				savedir = savedirView.getText().toString();
			}
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}
			public void afterTextChanged(Editable s) {
			}
		});
		datadirView = (EditText)this.findViewById(R.id.textDatadir);
		datadirView.setText(datadir);
		datadirView.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				datadir = datadirView.getText().toString();
			}
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}
			public void afterTextChanged(Editable s) {
			}
		});
		
		soundToggle = (ToggleButton)this.findViewById(R.id.toggleSound);
		if (sound.compareTo("yes") == 0)
			soundToggle.setChecked(true);
		else
			soundToggle.setChecked(false);
		soundToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				if (isChecked)
					sound = "yes";
				else
					sound = "no";
			}
		});
			
       	autosaveToggle = (ToggleButton)this.findViewById(R.id.toggleAutosave);
		if (autosave.compareTo("yes") == 0)
			autosaveToggle.setChecked(true);
		else
			autosaveToggle.setChecked(false);
		autosaveToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				if (isChecked)
					autosave = "yes";
				else
					autosave = "no";
			}
		});
		
       	startblankToggle = (ToggleButton)this.findViewById(R.id.toggleStartblank);
		if (startblank.compareTo("yes") == 0)
			startblankToggle.setChecked(true);
		else
			startblankToggle.setChecked(false);
		startblankToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				if (isChecked)
					startblank = "yes";
				else
					startblank = "no";
			}
		});
		
		
		locales = getResources().getStringArray(R.array.locales);
		int index = 0;
		for (; index != locales.length; index++){
			if (locales[index].compareTo(locale) == 0)
				break;
		}
		if(index==locales.length)
		    index = 0;
       	localeSpinner = (Spinner) findViewById(R.id.spinnerLocale);
		localeSpinner.setSelection(index); 
		localeSpinner.setOnItemSelectedListener(
                new AdapterView.OnItemSelectedListener() {
					public void onItemSelected(AdapterView<?> arg0, View arg1,
							int arg2, long arg3) {
						locale = locales[arg2];
					}
					public void onNothingSelected(AdapterView<?> arg0) {
					}
                });
		
			saveoverGroup = (RadioGroup)this.findViewById(R.id.groupSaveover);
			if (saveover.compareTo("yes") == 0)
				((RadioButton)this.findViewById(R.id.radioYes)).setChecked(true);
			else if (saveover.compareTo("ask") == 0)
				 ((RadioButton)this.findViewById(R.id.radioAsk)).setChecked(true);
			else
				((RadioButton)this.findViewById(R.id.radioNew)).setChecked(true);
			saveoverGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
				public void onCheckedChanged(RadioGroup group, int checkedId) {
					 int radioButtonId = group.getCheckedRadioButtonId();
					 if (radioButtonId == R.id.radioAsk)
						 saveover = "ask";
					 else if (radioButtonId == R.id.radioYes)
						 saveover = "yes";
					 else
						 saveover = "new";
				}
			});
	}
	
    @Override
    protected void onDestroy() {
        Log.v(TAG, "onDestroy()");
        save ();
        super.onDestroy();
    }
    
	private void load (){
	    	File internal = getFilesDir();
	     	File cfg =  new File (internal, "tuxpaint.cfg");
	    	try {
	    	InputStream in = new FileInputStream(cfg);
			props = new Properties();
	    	 props.load(in);
	    	 in.close();
	    	 } catch (Exception e1) {
	    	 // TODO Auto-generated catch block
	    	 e1.printStackTrace();
	    	 }
	    	 
	    	 autosave = props.getProperty("autosave", "no");
	    	 sound = props.getProperty("sound", "no");
	    	 saveover = props.getProperty("saveover", "ask");
	    	 startblank = props.getProperty("startblank", "no");
	    	 savedir = props.getProperty("savedir", internal.getAbsolutePath());
	    	 datadir = props.getProperty("datadir", internal.getAbsolutePath());
	    	 locale = props.getProperty("locale", Locale.getDefault().toString());
	    	 
	         Log.v(TAG, autosave + " " + sound + " " + saveover + " " + savedir+ " "+datadir+ " " + locale);;
	}
	
	private void save () {
    	File internal = getFilesDir();
     	File cfg =  new File (internal, "tuxpaint.cfg");
        props.put("autosave", autosave);
        props.put("sound", sound);
        props.put("saveover", saveover);
        props.put("startblank", startblank);
        props.put("savedir", savedir);
        props.put("datadir", datadir);
        props.put("locale", locale);

		try {
	 	  	OutputStream	out = new FileOutputStream(cfg);
	        props.store(out, "");
	        out.close();
		} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
		}
         
	}
    
}
