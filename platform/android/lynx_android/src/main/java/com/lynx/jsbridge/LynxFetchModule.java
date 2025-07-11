// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.jsbridge.network.HttpRequest;
import com.lynx.jsbridge.network.HttpResponse;
import com.lynx.jsbridge.network.HttpStreamingDelegate;
import com.lynx.jsbridge.network.LynxFetchModuleEventSender;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.LynxHttpRequestCallback;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.concurrent.atomic.AtomicLong;

public class LynxFetchModule extends LynxModule {
  public static final String NAME = "LynxFetchModule";
  private final LynxFetchModuleEventSender mSender;
  private static final AtomicLong streamingCounter = new AtomicLong();
  private static final String streamingEventNamePrefix = "LynxFetchModuleStreamingEvent";

  public LynxFetchModule(Context context, Object sender) {
    super(context);
    mSender = (LynxFetchModuleEventSender) sender;
  }

  private void request(
      ILynxHttpService httpService, HttpRequest httpRequest, String url, Callback resolve) {
    httpService.request(httpRequest, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        JavaOnlyMap resp = new JavaOnlyMap();
        resp.put("url", url);
        resp.put("body", response.getHttpBody() != null ? response.getHttpBody() : new byte[0]);
        resp.put("headers", response.getHttpHeaders() != null ? response.getHttpHeaders() : "");
        resp.put("status", response.getStatusCode());
        resp.put("statusText", response.getStatusText() != null ? response.getStatusText() : "");
        JavaOnlyMap customInfo =
            response.getCustomInfo() != null ? response.getCustomInfo() : new JavaOnlyMap();
        resp.put("lynxExtension", customInfo);
        resolve.invoke(resp);
      }
    });
  }

  private void requestStreaming(
      ILynxHttpService httpService, HttpRequest httpRequest, String url, Callback resolve) {
    String streamingId = streamingEventNamePrefix + streamingCounter.getAndIncrement();
    HttpStreamingDelegate delegate = new HttpStreamingDelegate(streamingId, mSender);

    httpService.requestStreaming(httpRequest, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        JavaOnlyMap resp = new JavaOnlyMap();
        resp.put("url", url);
        resp.put("body", new byte[0]);
        resp.put("headers", response.getHttpHeaders() != null ? response.getHttpHeaders() : "");
        resp.put("status", response.getStatusCode());
        resp.put("statusText", response.getStatusText() != null ? response.getStatusText() : "");
        JavaOnlyMap customInfo =
            response.getCustomInfo() != null ? response.getCustomInfo() : new JavaOnlyMap();
        customInfo.putString("streamingId", streamingId);
        resp.put("lynxExtension", customInfo);
        resolve.invoke(resp);
      }
    }, delegate);
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
    JavaOnlyMap customConfig = (JavaOnlyMap) request.getMap("lynxExtension", new JavaOnlyMap());
    httpRequest.setCustomConfig(customConfig);
    boolean useStreaming = customConfig.getBoolean("useStreaming", false);

    ILynxHttpService httpService = LynxServiceCenter.inst().getService(ILynxHttpService.class);
    if (httpService == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Lynx Http Service not registered");
      reject.invoke(error);
      return;
    }

    if (!useStreaming) {
      request(httpService, httpRequest, url, resolve);
    } else {
      requestStreaming(httpService, httpRequest, url, resolve);
    }
  }
}
