// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge.network;

import androidx.annotation.NonNull;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.LynxHttpRequestCallback;
import com.lynx.tasm.service.LynxServiceCenter;

public class LynxHttpRunner {
  public static final int SDK_ERROR_STATUS_CODE = 499;
  @CalledByNative
  public static boolean isHttpServiceRegistered() {
    ILynxHttpService httpService = LynxServiceCenter.inst().getService(ILynxHttpService.class);
    return httpService != null;
  }

  @CalledByNative
  public static void request(HttpRequest request, final Callback callback) {
    ILynxHttpService httpService = LynxServiceCenter.inst().getService(ILynxHttpService.class);
    if (httpService == null) {
      HttpResponse resp = new HttpResponse();
      resp.setStatusCode(SDK_ERROR_STATUS_CODE);
      resp.setStatusText("Lynx Http Service not registered");
      callback.invoke(resp);
      return;
    }

    httpService.request(request, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        callback.invoke(response);
      }
    });
  }
}
