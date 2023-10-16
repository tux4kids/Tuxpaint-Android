package org.tuxpaint;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatTextView;
import android.util.Log;
import android.view.View;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ImportData extends AppCompatActivity {
AppCompatButton selectButton = null;
AppCompatButton proceedButton = null;
Uri uri = null;
AppCompatTextView selected_data_TextView = null;
AppCompatTextView Feedback = null;
private static final String TAG = ImportData.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.importdata);


        selectButton = this.findViewById(R.id.buttonselect);
        proceedButton = this.findViewById(R.id.buttonproceed);
        selected_data_TextView = this.findViewById(R.id.textViewselected);
Feedback = this.findViewById(R.id.textViewFeedback);

        selectButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View buttonView) {
                select_file();
            }
        });
        
        
        proceedButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View buttonView) {
                uncompress("picked file");
            }
        });
    }


private void select_file() {

    Intent target = new Intent(Intent.ACTION_OPEN_DOCUMENT);
    target.setType("application/zip");
    target.addCategory(Intent.CATEGORY_OPENABLE);
    final Intent intent = Intent.createChooser(target, getString(R.string.document_choose));
    startActivityForResult(intent, 1);
}





    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1) {
            if (resultCode == Activity.RESULT_OK) {
                if (data != null) {
                    uri = data.getData();
                    Log.i(TAG, "Uri: " + uri.toString());
                    selected_data_TextView.setText(uri.toString());
                }
            }
        }
    }




    public void uncompress(String action) {
        /* Unzip code adapted from the code made by Jianwei Zhang in GSoC 2015 */
        String filename = "stamps.zip";
        ArrayList<String> arraydir = new ArrayList<String>();
        InputStream in = null;
        File zipfile = null;
        Log.i(TAG, "In uncompres, Uri: " + uri.toString());
        /* FIXME: Get the file path from what is selected for data directory in the config activity. */
        File internal = new File("/storage/emulated/0/Android/data/org.tuxpaint/files");
        try {
            if (action.equals("from assets")) {
                zipfile = new File(filename);
                in = getAssets().open(filename);
            }
            else {

                in = getContentResolver().openInputStream(uri);
            }
            ZipInputStream zin;
            String name;
            zin = new ZipInputStream(new BufferedInputStream(in));
            ZipEntry ze;
            byte[] buffer = new byte[1024];
            int count;



            while ((ze = zin.getNextEntry()) != null) {
                name = ze.getName();

                if (ze.isDirectory()) {
                    File dir = new File(internal, name);

                        dir.mkdirs();

                    continue;
                }

                File file = new File(internal, name);

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





                    setFeedback(R.string.Files_installed);



        } catch (IOException e) {
            e.printStackTrace();
                setFeedback(R.string.Files_notinstalled);
        }
    }

public void setFeedback(int feedback) {
       runOnUiThread(new Runnable() {
           @Override
           public void run() {
               Feedback.setText(feedback);
           }
       }
       );
    }




}