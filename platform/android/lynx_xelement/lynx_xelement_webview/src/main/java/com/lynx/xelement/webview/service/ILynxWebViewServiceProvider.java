// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview.service;

import android.content.Context;

public interface ILynxWebViewServiceProvider {
  void registerService(String playerType, ILynxWebViewService service);

  void unRegisterService(String playerType);

  ILynxWebViewService getLynxWebViewService(String playerType, Context context);
}
