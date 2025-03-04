// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.jsbridge.network.HttpRequest;
import com.lynx.jsbridge.network.HttpResponse;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.LynxHttpRequestCallback;
import com.lynx.tasm.service.LynxServiceCenter;

public class LynxFetchModule extends LynxModule {
  public static final String NAME = "LynxFetchModule";

  public LynxFetchModule(Context context) {
    super(context);
  }

  @LynxMethod
  public void fetch(final ReadableMap request, final Callback resolve, final Callback reject) {
    String url = request.getString("url", "");

    HttpRequest httpRequest = new HttpRequest();
    httpRequest.setHttpMethod(request.getString("method", ""));
    httpRequest.setUrl(url);
    httpRequest.setOriginUrl(request.getString("origin", ""));
    httpRequest.setHttpHeaders((JavaOnlyMap) request.getMap("headers", new JavaOnlyMap()));
    httpRequest.setHttpBody(request.getByteArray("body", new byte[0]));
    httpRequest.setCustomConfig((JavaOnlyMap) request.getMap("lynxExtension", new JavaOnlyMap()));

    ILynxHttpService httpService = LynxServiceCenter.inst().getService(ILynxHttpService.class);
    if (httpService == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Lynx Http Service not registered");
      reject.invoke(error);
      return;
    }

    httpService.request(httpRequest, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        JavaOnlyMap resp = new JavaOnlyMap();
        resp.put("url", url);
        resp.put("body", response.getHttpBody() != null ? response.getHttpBody() : new byte[0]);
        resp.put("headers", response.getHttpHeaders() != null ? response.getHttpHeaders() : "");
        resp.put("status", response.getStatusCode());
        resp.put("statusText", response.getStatusText() != null ? response.getStatusText() : "");
        resp.put("lynxExtension",
            response.getCustomInfo() != null ? response.getCustomInfo() : new JavaOnlyMap());
        resolve.invoke(resp);
      }
    });
  }
}
