// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview;

import android.content.Context;
import android.os.Build;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import com.lynx.xelement.webview.service.ILynxWebViewCallback;
import com.lynx.xelement.webview.service.ILynxWebViewService;
import java.util.HashMap;

public class DefaultWebViewServiceImpl implements ILynxWebViewService {
  private final Context context;
  private ILynxWebViewCallback callback;
  private WebView webView;

  public DefaultWebViewServiceImpl(Context context) {
    this.context = context;
  }

  @Override
  public void setCallback(ILynxWebViewCallback callback) {
    this.callback = callback;
  }

  @Override
  public void setParams(HashMap<String, Object> params) {
    // The default WebView implementation does not consume custom params.
  }

  @Override
  public void initWebView() {
    webView = new WebView(context);
    WebSettings settings = webView.getSettings();
    settings.setJavaScriptEnabled(true);
    settings.setAllowFileAccess(false);
    settings.setAllowContentAccess(false);
    settings.setJavaScriptCanOpenWindowsAutomatically(true);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
    }
    settings.setDomStorageEnabled(true);
    settings.setUseWideViewPort(true);
    settings.setLoadWithOverviewMode(true);

    webView.addJavascriptInterface(this, "LynxWebViewBridge");
    webView.setWebViewClient(new WebViewClient() {
      @Override
      public void onPageFinished(WebView view, String url) {
        super.onPageFinished(view, url);
        if (callback != null) {
          callback.onPageFinished(view, url);
        }
      }

      @Override
      public void onReceivedError(
          WebView view, WebResourceRequest request, WebResourceError error) {
        super.onReceivedError(view, request, error);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && callback != null && error != null) {
          callback.onReceivedError(
              view, request, error.getErrorCode(), error.getDescription().toString());
        }
      }
    });
  }

  @JavascriptInterface
  public void onMessageReceived(String message) {
    if (callback != null) {
      callback.onMessageReceived(message);
    }
  }

  @Override
  public WebView getWebView() {
    return webView;
  }

  @Override
  public void loadUrl(String url) {
    if (webView != null) {
      webView.loadUrl(url);
    }
  }

  @Override
  public void loadHtmlString(String htmlString) {
    if (webView != null) {
      webView.loadDataWithBaseURL(null, htmlString, "text/html", "utf-8", null);
    }
  }

  @Override
  public void reload() {
    if (webView != null) {
      webView.reload();
    }
  }

  @Override
  public void evaluateJavascript(String script, ValueCallback<String> result) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT && webView != null) {
      webView.evaluateJavascript(script, result);
    }
  }

  @Override
  public void destroy() {
    if (webView != null) {
      webView.destroy();
      webView = null;
    }
  }
}
