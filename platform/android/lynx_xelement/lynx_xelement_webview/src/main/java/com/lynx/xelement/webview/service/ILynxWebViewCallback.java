// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview.service;

import android.webkit.WebResourceRequest;
import android.webkit.WebView;

public interface ILynxWebViewCallback {
  void onPageFinished(WebView view, String url);

  void onReceivedError(
      WebView view, WebResourceRequest request, int errorCode, String errorMessage);

  void onMessageReceived(String message);
}
