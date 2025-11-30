package org.tuxpaint;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.view.KeyEvent;
import java.util.Locale;
import java.io.IOException;

public class DocViewerActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.doc_viewer);

        WebView webView = (WebView) findViewById(R.id.webView);
        WebSettings webSettings = webView.getSettings();
        webSettings.setBuiltInZoomControls(true);
        webSettings.setDisplayZoomControls(false);
        webSettings.setLoadWithOverviewMode(true);
        webSettings.setUseWideViewPort(true);

        webView.setWebViewClient(new WebViewClient());

        String lang = Locale.getDefault().getLanguage();
        String url = "file:///android_asset/docs/en/html/README.html";
        String targetFolder = null;

        if (lang.equals("es")) targetFolder = "es_ES.UTF-8";
        else if (lang.equals("fr")) targetFolder = "fr_FR.UTF-8";
        else if (lang.equals("gl")) targetFolder = "gl_ES.UTF-8";
        else if (lang.equals("is")) targetFolder = "is_IS.UTF-8";
        else if (lang.equals("ja")) targetFolder = "ja_JP.UTF-8";
        else if (lang.equals("sq")) targetFolder = "sq_AL.UTF-8";
        else if (lang.equals("sv")) targetFolder = "sv_SE.UTF-8";

        if (targetFolder != null) {
             try {
                 String[] list = getAssets().list("docs/" + targetFolder);
                 if (list != null && list.length > 0) {
                     url = "file:///android_asset/docs/" + targetFolder + "/html/README.html";
                 }
             } catch (IOException e) {
                 // Folder doesn't exist or error, fallback to en
             }
        }

        webView.loadUrl(url);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_BACK:
                    WebView webView = (WebView) findViewById(R.id.webView);
                    if (webView.canGoBack()) {
                        webView.goBack();
                    } else {
                        finish();
                    }
                    return true;
            }

        }
        return super.onKeyDown(keyCode, event);
    }
}
