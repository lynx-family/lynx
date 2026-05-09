// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview.service;

import android.webkit.ValueCallback;
import android.webkit.WebView;
import java.util.HashMap;

public interface ILynxWebViewService {
  void setCallback(ILynxWebViewCallback callback);

  void setParams(HashMap<String, Object> params);

  void initWebView();

  WebView getWebView();

  void loadUrl(String url);

  void loadHtmlString(String htmlString);

  void reload();

  void evaluateJavascript(String script, ValueCallback<String> result);

  void destroy();
}
