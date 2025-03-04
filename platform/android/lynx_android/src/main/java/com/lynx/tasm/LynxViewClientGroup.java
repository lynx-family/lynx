// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.view.KeyEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import javax.xml.transform.Transformer;

public class LynxViewClientGroup extends LynxViewClient {
  private CopyOnWriteArrayList<LynxViewClient> mClients = new CopyOnWriteArrayList<>();

  private static final String TRACE_LYNXVIEW_AND_JSRUNTIME_DESTORY =
      "Client.onLynxViewAndJSRuntimeDestroy";
  private static final String TRACE_PIPER_INVOKED = "Client.onPiperInvoked";
  private static final String TRACE_DESTORY = "Client.onDestory";
  private static final String TRACE_TIMING_SETUP = "Client.onTimingSetup";
  private static final String TRACE_TIMING_UPDATE = "Client.onTimingUpdate";
  private static final String TRACE_SCROLL_STOP = "Client.onScrollStop";
  private static final String TRACE_SCROLL_START = "Client.onScrollStart";
  private static final String TRACE_FLING = "Client.onFling";
  protected static final String TRACE_CLIENT_ON_FIRST_SCREEN = "Client.onFirstScreen";
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
  private boolean enableLifecycleTimeReport = false;

  public void addClient(LynxViewClient client) {
    if (!mClients.contains(client)) {
      mClients.add(client);
    }
  }

  public void removeClient(LynxViewClient client) {
    mClients.remove(client);
  }

  public void setEnableLifecycleTimeReport(boolean enabled) {
    enableLifecycleTimeReport = enabled;
  }

  public void setInstanceId(int id) {
    mInstanceId = id;
  }

  // Some callbacks are very frequent, so we need to directly record the start time as a parameter
  // during execution, rather than encapsulating the entire execution content into a Runnable for
  // cleaner code style.
  private void recordLifecycleTimeWithStartTime(String callbackName, long startTime) {
    if (mInstanceId == LynxEventReporter.INSTANCE_ID_UNKNOWN) {
      return;
    }

    long overallEndTime = System.nanoTime();
    double overallMillsDuration = (overallEndTime - startTime) / 1000000d;

    String threadName = Thread.currentThread().getName();
    LynxEventReporter.PropsBuilder builder = () -> {
      Map<String, Object> props = new HashMap<>();
      props.put("name", callbackName);
      props.put("duration", overallMillsDuration);
      props.put("thread", threadName);
      return props;
    };
    LynxEventReporter.onEvent("lynxsdk_lifecycle_time", mInstanceId, builder);
  }

  @Override
  public void onPageStart(String url) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onPageStart(url);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onPageStart", startTime);
    }
  }

  @Override
  public void onLoadSuccess() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onLoadSuccess();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onLoadSuccess", startTime);
    }
  }

  @Override
  public void onLoadFailed(String message) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onLoadFailed(message);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onLoadFailed", startTime);
    }
  }

  @Override
  public void onFirstScreen() {
    long startTime = -1;
    TraceEvent.beginSection(TRACE_CLIENT_ON_FIRST_SCREEN);
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onFirstScreen();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onFirstScreen", startTime);
    }
    TraceEvent.endSection(TRACE_CLIENT_ON_FIRST_SCREEN);
  }

  @Override
  public void onPageUpdate() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onPageUpdate();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onPageUpdate", startTime);
    }
  }

  @Override
  public void onDataUpdated() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onDataUpdated();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onDataUpdated", startTime);
    }
  }

  @Override
  public void onTASMFinishedByNative() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onTASMFinishedByNative();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onTASMFinishedByNative", startTime);
    }
  }

  @Override
  public void onReceivedError(String info) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      client.onReceivedError(info);
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReceivedError", startTime);
    }
  }

  @Override
  public void onReceivedJSError(LynxError jsError) {
    // Client will retrieve error info through LynxError.getMsg().
    // We invoke getMsg() to pre-generate the json string cache
    // for LynxError to avoid race conditions in clients.
    if (jsError == null || jsError.getMsg() == null) {
      return;
    }

    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      client.onReceivedJSError(jsError);
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReceivedJSError", startTime);
    }
  }

  @Override
  public void onReceivedNativeError(LynxError nativeError) {
    // Client will retrieve error info through LynxError.getMsg().
    // We invoke getMsg() to pre-generate the json string cache
    // for LynxError to avoid race conditions in clients.
    if (nativeError == null || nativeError.getMsg() == null) {
      return;
    }

    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      client.onReceivedNativeError(nativeError);
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReceivedNativeError", startTime);
    }
  }

  public void onReceivedError(LynxError error) {
    // Client will retrieve error info through LynxError.getMsg().
    // We invoke getMsg() to pre-generate the json string cache
    // for LynxError to avoid race conditions in clients.
    if (error == null || error.getMsg() == null) {
      return;
    }

    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      client.onReceivedError(error);
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReceivedError", startTime);
    }
  }

  @Override
  public void onReceivedJavaError(LynxError error) {
    // Client will retrieve error info through LynxError.getMsg().
    // We invoke getMsg() to pre-generate the json string cache
    // for LynxError to avoid race conditions in clients.
    if (error == null || error.getMsg() == null) {
      return;
    }

    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      client.onReceivedJavaError(error);
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReceivedJavaError", startTime);
    }
  }

  @Override
  public void onRuntimeReady() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onRuntimeReady();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onRuntimeReady", startTime);
    }
  }

  @Override
  public void onFirstLoadPerfReady(LynxPerfMetric metric) {}

  @Override
  public void onUpdatePerfReady(LynxPerfMetric metric) {}

  @Override
  public void onDynamicComponentPerfReady(HashMap<String, Object> perf) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onDynamicComponentPerfReady(perf);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onDynamicComponentPerfReady", startTime);
    }
  }

  @Override
  public void onReportComponentInfo(Set<String> mComponentSet) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onReportComponentInfo(mComponentSet);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onReportComponentInfo", startTime);
    }
  }

  @Override
  public void onDestroy() {
    TraceEvent.beginSection(TRACE_DESTORY);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onDestroy();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onDestroy", startTime);
    }
    TraceEvent.endSection(TRACE_DESTORY);
  }

  @Override
  public String shouldRedirectImageUrl(String url) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }

    for (LynxViewClient client : mClients) {
      String redirectUrl = client.shouldRedirectImageUrl(url);
      if (redirectUrl != null) {
        if (startTime != -1) {
          recordLifecycleTimeWithStartTime("shouldRedirectImageUrl", startTime);
        }
        return redirectUrl;
      }
    }

    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("shouldRedirectImageUrl", startTime);
    }

    return null;
  }

  @Override
  public void loadImage(@NonNull Context context, @Nullable String cacheKey, @Nullable String src,
      float width, float height, final @Nullable Transformer transformer,
      @NonNull final CompletionHandler handler) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.loadImage(context, cacheKey, src, width, height, transformer, handler);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("loadImage", startTime);
    }
  }

  @Override
  public void onUpdateDataWithoutChange() {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onUpdateDataWithoutChange();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onUpdateDataWithoutChange", startTime);
    }
  }

  // issue: #1510
  @Override
  public void onModuleMethodInvoked(String module, String method, int error_code) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onModuleMethodInvoked(module, method, error_code);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onModuleMethodInvoked", startTime);
    }
  }

  @Override
  public void onPiperInvoked(Map<String, Object> info) {
    TraceEvent.beginSection(TRACE_PIPER_INVOKED);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onPiperInvoked(info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onPiperInvoked", startTime);
    }
    TraceEvent.endSection(TRACE_PIPER_INVOKED);
  }

  @Override
  public void onLynxViewAndJSRuntimeDestroy() {
    TraceEvent.beginSection(TRACE_LYNXVIEW_AND_JSRUNTIME_DESTORY);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onLynxViewAndJSRuntimeDestroy();
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onLynxViewAndJSRuntimeDestroy", startTime);
    }
    TraceEvent.endSection(TRACE_LYNXVIEW_AND_JSRUNTIME_DESTORY);
  }

  @Override
  public void onScrollStart(ScrollInfo info) {
    TraceEvent.beginSection(TRACE_SCROLL_START);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onScrollStart(info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onScrollStart", startTime);
    }
    TraceEvent.endSection(TRACE_SCROLL_START);
  }

  @Override
  public void onScrollStop(ScrollInfo info) {
    TraceEvent.beginSection(TRACE_SCROLL_STOP);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onScrollStop(info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onScrollStop", startTime);
    }
    TraceEvent.endSection(TRACE_SCROLL_STOP);
  }

  @Override
  public void onFling(ScrollInfo info) {
    TraceEvent.beginSection(TRACE_FLING);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onFling(info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onFling", startTime);
    }
    TraceEvent.endSection(TRACE_FLING);
  }

  @Override
  public void onFlushFinish(FlushInfo info) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onFlushFinish(info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onFlushFinish", startTime);
    }
  }

  @Override
  public void onKeyEvent(KeyEvent event, boolean handled) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onKeyEvent(event, handled);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onKeyEvent", startTime);
    }
  }

  @Override
  public void onTimingUpdate(
      Map<String, Object> timingInfo, Map<String, Long> updateTiming, String flag) {
    TraceEvent.beginSection(TRACE_TIMING_UPDATE);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onTimingUpdate(timingInfo, updateTiming, flag);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onTimingUpdate", startTime);
    }
    TraceEvent.endSection(TRACE_TIMING_UPDATE);
  }

  @Override
  public void onTimingSetup(Map<String, Object> timingInfo) {
    TraceEvent.beginSection(TRACE_TIMING_SETUP);
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onTimingSetup(timingInfo);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onTimingSetup", startTime);
    }
    TraceEvent.endSection(TRACE_TIMING_SETUP);
  }

  @Override
  public void onJSBInvoked(Map<String, Object> jsbInfo) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onJSBInvoked(jsbInfo);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onJSBInvoked", startTime);
    }
  }

  @Override
  public void onCallJSBFinished(Map<String, Object> jsbTiming) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onCallJSBFinished(jsbTiming);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onCallJSBFinished", startTime);
    }
  }

  @Override
  public void onLynxEvent(LynxEventDetail detail) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onLynxEvent(detail);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onLynxEvent", startTime);
    }
  }

  @Override
  public void onTemplateBundleReady(TemplateBundle bundle) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClient client : mClients) {
      client.onTemplateBundleReady(bundle);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onTemplateBundleReady", startTime);
    }
  }
}
