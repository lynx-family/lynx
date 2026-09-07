// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.jsbridge.network.HttpRequest;
import com.lynx.jsbridge.network.HttpResponse;
import com.lynx.jsbridge.network.HttpStreamingDelegate;
import com.lynx.jsbridge.network.LynxFetchModuleEventSender;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.LynxHttpRequestCallback;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

public class LynxFetchModule extends LynxModule {
  public static final String NAME = "LynxFetchModule";
  private final LynxFetchModuleEventSender mSender;
  private static final AtomicLong streamingCounter = new AtomicLong();
  private static final String streamingEventNamePrefix = "LynxFetchModuleStreamingEvent";
  private static final String deprecatedStreamingFlag = "useStreaming";
  private static final String standardStreamingFlag = "enableFetchAPIStandardStreaming";

  public LynxFetchModule(Context context, Object sender) {
    super(context);
    mSender = (LynxFetchModuleEventSender) sender;
  }

  private void request(ILynxHttpService httpService, HttpRequest httpRequest, String url,
      Callback resolve, LynxNetworkRequestObserver networkObserver, String networkRequestId) {
    httpService.request(httpRequest, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        byte[] responseBody = response.getHttpBody() != null ? response.getHttpBody() : new byte[0];
        JavaOnlyMap responseHeaders = response.getHttpHeaders();
        String statusText = response.getStatusText() != null ? response.getStatusText() : "";

        if (!networkRequestId.isEmpty()) {
          // A normal Fetch exposes the complete response in one callback. Keep
          // the CDP lifecycle ordering explicit while reusing the common
          // observer state machine for validation, caching, and event output.
          networkObserver.responseReceived(networkRequestId, response.getUrl(),
              response.getStatusCode(), statusText, responseHeaders);
          networkObserver.dataReceived(networkRequestId, responseBody);
          networkObserver.loadingFinished(networkRequestId);
        }

        JavaOnlyMap resp = new JavaOnlyMap();
        resp.put("url", url);
        resp.put("body", responseBody);
        resp.put("headers", responseHeaders != null ? responseHeaders : "");
        resp.put("status", response.getStatusCode());
        resp.put("statusText", statusText);
        JavaOnlyMap customInfo =
            response.getCustomInfo() != null ? response.getCustomInfo() : new JavaOnlyMap();
        resp.put("lynxExtension", customInfo);
        resolve.invoke(resp);
      }
    });
  }

  private void requestStreaming(ILynxHttpService httpService, HttpRequest httpRequest, String url,
      Callback resolve, LynxNetworkRequestObserver networkObserver, String networkRequestId) {
    String streamingId = streamingEventNamePrefix + streamingCounter.getAndIncrement();
    HttpStreamingDelegate delegate =
        new HttpStreamingDelegate(streamingId, mSender, networkObserver, networkRequestId);

    httpService.requestStreaming(httpRequest, new LynxHttpRequestCallback() {
      @Override
      public void invoke(@NonNull HttpResponse response) {
        JavaOnlyMap responseHeaders = response.getHttpHeaders();
        String statusText = response.getStatusText() != null ? response.getStatusText() : "";
        if (!networkRequestId.isEmpty()) {
          networkObserver.responseReceived(networkRequestId, response.getUrl(),
              response.getStatusCode(), statusText, responseHeaders);
        }

        JavaOnlyMap resp = new JavaOnlyMap();
        resp.put("url", url);
        resp.put("body", new byte[0]);
        resp.put("headers", responseHeaders != null ? responseHeaders : "");
        resp.put("status", response.getStatusCode());
        resp.put("statusText", statusText);
        JavaOnlyMap customInfo =
            response.getCustomInfo() != null ? response.getCustomInfo() : new JavaOnlyMap();
        customInfo.putString("streamingId", streamingId);
        resp.put("lynxExtension", customInfo);
        resolve.invoke(resp);
      }
    }, delegate);
  }

  private void traceFetchRequest(String url) {
    if (!TraceEvent.isTracingStarted()) {
      return;
    }
    Map<String, String> props = new HashMap<>();
    props.put("url", url != null ? url : "");
    props.put("module_name", NAME);
    props.put("method_name", "fetch");
    TraceEvent.instant(
        TraceEvent.CATEGORY_DEFAULT, TraceEventDef.NATIVE_MODULE_NETWORK_REQUEST, props);
  }

  @LynxMethod
  public void fetch(final ReadableMap request, final Callback resolve, final Callback reject) {
    String url = request.getString("url", "");
    traceFetchRequest(url);

    HttpRequest httpRequest = new HttpRequest();
    httpRequest.setHttpMethod(request.getString("method", ""));
    httpRequest.setUrl(url);
    httpRequest.setOriginUrl(request.getString("origin", ""));
    httpRequest.setHttpHeaders((JavaOnlyMap) request.getMap("headers", new JavaOnlyMap()));
    httpRequest.setHttpBody(request.getByteArray("body", new byte[0]));
    JavaOnlyMap customConfig = (JavaOnlyMap) request.getMap("lynxExtension", new JavaOnlyMap());
    httpRequest.setCustomConfig(customConfig);
    boolean deprecatedUseStreaming = customConfig.getBoolean(deprecatedStreamingFlag, false);
    boolean enableFetchApiStandardStreaming = customConfig.getBoolean(standardStreamingFlag, false);
    boolean useStreaming = deprecatedUseStreaming || enableFetchApiStandardStreaming;

    LynxNetworkRequestObserver networkObserver = mSender.getNetworkRequestObserver();
    String networkRequestId = "";
    if (networkObserver != null && networkObserver.isEnabled()) {
      networkRequestId = networkObserver.requestWillBeSent(httpRequest.getUrl(),
          httpRequest.getHttpMethod(), httpRequest.getHttpHeaders(), httpRequest.getHttpBody());
    }

    ILynxHttpService httpService = LynxServiceCenter.inst().getService(ILynxHttpService.class);
    if (httpService == null) {
      if (!networkRequestId.isEmpty()) {
        networkObserver.loadingFailed(networkRequestId, "Lynx Http Service not registered", false);
      }
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Lynx Http Service not registered");
      reject.invoke(error);
      return;
    }

    if (!useStreaming) {
      request(httpService, httpRequest, url, resolve, networkObserver, networkRequestId);
    } else {
      requestStreaming(httpService, httpRequest, url, resolve, networkObserver, networkRequestId);
    }
  }
}
