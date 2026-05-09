// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview;

import android.content.Context;
import com.lynx.xelement.webview.service.ILynxWebViewService;
import com.lynx.xelement.webview.service.ILynxWebViewServiceProvider;
import java.util.concurrent.ConcurrentHashMap;

public class LynxWebViewServiceProviderImpl implements ILynxWebViewServiceProvider {
  private ILynxWebViewService defaultWebViewService;
  private final ConcurrentHashMap<String, ILynxWebViewService> webViewServiceRegistry =
      new ConcurrentHashMap<>();

  @Override
  public void registerService(String playerType, ILynxWebViewService service) {
    if (service != null) {
      webViewServiceRegistry.put(playerType, service);
    }
  }

  @Override
  public void unRegisterService(String playerType) {
    webViewServiceRegistry.remove(playerType);
  }

  @Override
  public ILynxWebViewService getLynxWebViewService(String playerType, Context context) {
    if (LynxUIWebView.DEFAULT.equals(playerType)) {
      if (defaultWebViewService == null) {
        defaultWebViewService = new DefaultWebViewServiceImpl(context);
      }
      return defaultWebViewService;
    }
    return webViewServiceRegistry.get(playerType);
  }
}
