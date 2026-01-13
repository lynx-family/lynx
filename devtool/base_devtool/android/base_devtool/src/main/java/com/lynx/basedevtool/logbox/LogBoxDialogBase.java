// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.app.Dialog;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Build;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.LinearLayout;
import com.lynx.basedevtool.utils.DevToolDownloader;
import com.lynx.basedevtool.utils.DownloadCallback;
import com.lynx.basedevtool.utils.UIThreadUtils;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.Charset;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

abstract public class LogBoxDialogBase extends Dialog {
  private static final String TAG = "LogBoxDialogBase";
  protected static final String LOCAL_URL = "file:///android_asset/logbox/index.html";
  private static final float CONTENT_HEIGHT_PERCENT = 0.6f;

  private static final String BRIDGE_JS =
      new StringBuilder()
          .append("if (!window.logbox) {")
          .append("  (function () {")
          .append("    var id = 0, callbacks = {}, eventListeners = {};")
          .append(
              "    var nativeBridge = window.nativeBridge || window.webkit.messageHandlers.nativeBridge;")
          .append("    window.logbox = {")
          .append("      call: function(bridgeName, callback, data) {")
          .append("        var thisId = id++;")
          .append("        callbacks[thisId] = callback;")
          .append("        nativeBridge.postMessage(JSON.stringify({")
          .append("          bridgeName: bridgeName,")
          .append("          data: data ? JSON.stringify(data) : {},")
          .append("          callbackId: thisId")
          .append("        }));")
          .append("      },")
          .append("      on: function(event, handler) {")
          .append("        eventListeners[event] = handler;")
          .append("      },")
          .append("      sendResult: function(msg) {")
          .append("        var callbackId = msg.callbackId;")
          .append("        if (callbacks[callbackId]) {")
          .append("          callbacks[callbackId](msg.data);")
          .append("        }")
          .append("      },")
          .append("      sendEvent: function(msg) {")
          .append("        if (eventListeners[msg.event]) {")
          .append("          eventListeners[msg.event](msg.data);")
          .append("        }")
          .append("      }")
          .append("    };")
          .append("  })();")
          .append("  setTimeout(function(){document.dispatchEvent(new Event('LogBoxReady'))}, 0);")
          .append("};")
          .toString();

  private WebView mWebView;

  protected Map<String, Object> mJsSource;

  private LinearLayout mRootLayout;

  public class Callback {
    @JavascriptInterface
    public void on(String event, Object handler) {
      Log.e(TAG, "onEvent " + event);
    }

    public void sendResult(int callbackId, Object result) {
      if (result == null) {
        return;
      }
      JSONObject obj = new JSONObject();
      try {
        obj.put("callbackId", callbackId);
        obj.put("data", result);
      } catch (JSONException e) {
        Log.e(TAG, e.getMessage());
        return;
      }

      StringBuilder strBuilder = new StringBuilder();
      strBuilder.append("javascript: window.logbox.sendResult(")
          .append(obj.toString())
          .append(");");
      evaluateJs(strBuilder.toString());
    }

    public void download(String url, DownloadCallback callback) {
      if (callback != null && !TextUtils.isEmpty(url) && url.startsWith("http")) {
        DevToolDownloader downloader = new DevToolDownloader(url, callback);
      }
    }

    public void loadMappingsWasm() {
      AssetManager manager = getContext().getAssets();
      try {
        InputStream stream = manager.open("logbox/mappings.wasm");
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        byte[] buf = new byte[8192];
        for (;;) {
          int readBytes = stream.read(buf);
          if (readBytes == -1) {
            break;
          }
          buffer.write(buf, 0, readBytes);
        }
        stream.close();
        String base64Data = Base64.encodeToString(buffer.toByteArray(), Base64.DEFAULT);
        JSONObject event = new JSONObject();
        event.put("event", "loadFile");
        JSONObject dataObject = new JSONObject();
        dataObject.putOpt("type", "mappings.wasm");
        dataObject.putOpt("data", base64Data);
        event.putOpt("data", dataObject);
        sendEvent(event);
      } catch (IOException | JSONException e) {
        Log.w("Failed to load mappings.wasm: ", e.getMessage());
      }
    }
  }

  protected LogBoxDialogBase(Context context) {
    super(context);
    requestWindowFeature(Window.FEATURE_NO_TITLE);

    mRootLayout = new LinearLayout(context);
    mRootLayout.setLayoutParams(new LinearLayout.LayoutParams(
        LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT));
    mRootLayout.setBackgroundColor(Color.alpha(80));
    mRootLayout.setOrientation(LinearLayout.VERTICAL);
    setContentView(mRootLayout);

    mRootLayout.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        reset();
        dismiss();
      }
    });

    mWebView = new WebView(context);

    LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
        LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
    int screenHeight = context.getResources().getDisplayMetrics().heightPixels;
    layoutParams.setMargins(0, (int) Math.round(screenHeight * (1 - CONTENT_HEIGHT_PERCENT)), 0, 0);
    mRootLayout.addView(mWebView, layoutParams);

    if (getWindow() != null) {
      getWindow().setBackgroundDrawableResource(android.R.color.transparent);
      getWindow().setLayout(
          WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT);
    }
  }

  public void initWebView(Callback callbacks) {
    mWebView.getSettings().setJavaScriptEnabled(true);
    mWebView.getSettings().setUserAgentString(
        mWebView.getSettings().getUserAgentString() + " Lynx LogBox");
    mWebView.setWebViewClient(new WebViewClient() {
      @Override
      public void onReceivedError(
          WebView view, WebResourceRequest request, WebResourceError error) {
        super.onReceivedError(view, request, error);
        if (!isLoadingFinished()) {
          Log.i(TAG, "onReceivedError when load log box");
        }
      }
      @Override
      public void onPageFinished(WebView view, String url) {
        super.onPageFinished(view, url);
        evaluateJs(BRIDGE_JS);
      }

      @Override
      public void onReceivedHttpError(
          WebView view, WebResourceRequest request, WebResourceResponse errorResponse) {
        super.onReceivedHttpError(view, request, errorResponse);
        if (!isLoadingFinished()) {
          Log.i(TAG, "onReceivedHttpError when load log box");
        }
      }
    });
    mWebView.setWebChromeClient(new WebChromeClient() {
      @Override
      public void onReceivedTitle(WebView view, String title) {
        super.onReceivedTitle(view, title);
        evaluateJs(BRIDGE_JS);
      }
    });
    mWebView.addJavascriptInterface(callbacks, "nativeBridge");
    // If debugging the webview is required,
    // add setWebContentsDebuggingEnabled(true) here
    mWebView.loadUrl(LOCAL_URL);
  }

  public void setJSSource(Map<String, Object> jsSource) {
    mJsSource = jsSource;
  }

  protected void evaluateJs(String js) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (Build.VERSION.SDK_INT >= 19) {
          mWebView.evaluateJavascript(js, null);
        } else {
          mWebView.loadUrl(js);
        }
      }
    });
  }

  protected void sendEvent(JSONObject event) {
    final String js = "javascript: window.logbox.sendEvent(" + event.toString() + ");";
    evaluateJs(js);
  }

  protected void sendEvent(String event) {
    final String js = "javascript: window.logbox.sendEvent({event: \"" + event + "\"});";
    evaluateJs(js);
  }

  public void destroyWebView() {
    mWebView.destroy();
  }

  abstract public void reset();
  abstract public boolean isLoadingFinished();
}
