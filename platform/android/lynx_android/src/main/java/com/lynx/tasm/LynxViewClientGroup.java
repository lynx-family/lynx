// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.view.KeyEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import javax.xml.transform.Transformer;

public class LynxViewClientGroup extends LynxViewClient {
  private CopyOnWriteArrayList<LynxViewClient> mClients = new CopyOnWriteArrayList<>();
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;

  public void addClient(LynxViewClient client) {
    if (!mClients.contains(client)) {
      mClients.add(client);
    }
  }

  public void removeClient(LynxViewClient client) {
    mClients.remove(client);
  }

  public void setInstanceId(int id) {
    mInstanceId = id;
  }

  @Override
  public void onPageStart(String url) {
    for (LynxViewClient client : mClients) {
      client.onPageStart(url);
    }
  }

  @Override
  public void onLoadSuccess() {
    for (LynxViewClient client : mClients) {
      client.onLoadSuccess();
    }
  }

  @Override
  public void onLoadFailed(String message) {
    for (LynxViewClient client : mClients) {
      client.onLoadFailed(message);
    }
  }

  @Override
  public void onFirstScreen() {
    TraceEvent.beginSection(TraceEventDef.CLIENT_ON_FIRST_SCREEN);
    for (LynxViewClient client : mClients) {
      client.onFirstScreen();
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_ON_FIRST_SCREEN);
  }

  @Override
  public void onPageUpdate() {
    for (LynxViewClient client : mClients) {
      client.onPageUpdate();
    }
  }

  @Override
  public void onDataUpdated() {
    for (LynxViewClient client : mClients) {
      client.onDataUpdated();
    }
  }

  @Override
  public void onTASMFinishedByNative() {
    for (LynxViewClient client : mClients) {
      client.onTASMFinishedByNative();
    }
  }

  @Override
  public void onReceivedError(String info) {
    for (LynxViewClient client : mClients) {
      client.onReceivedError(info);
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
    for (LynxViewClient client : mClients) {
      client.onReceivedJSError(jsError);
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
    for (LynxViewClient client : mClients) {
      client.onReceivedNativeError(nativeError);
    }
  }

  public void onReceivedError(LynxError error) {
    // Client will retrieve error info through LynxError.getMsg().
    // We invoke getMsg() to pre-generate the json string cache
    // for LynxError to avoid race conditions in clients.
    if (error == null || error.getMsg() == null) {
      return;
    }
    for (LynxViewClient client : mClients) {
      client.onReceivedError(error);
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
    for (LynxViewClient client : mClients) {
      client.onReceivedJavaError(error);
    }
  }

  @Override
  public void onRuntimeReady() {
    for (LynxViewClient client : mClients) {
      client.onRuntimeReady();
    }
  }

  @Override
  public void onFirstLoadPerfReady(LynxPerfMetric metric) {}

  @Override
  public void onUpdatePerfReady(LynxPerfMetric metric) {}

  @Override
  public void onDynamicComponentPerfReady(HashMap<String, Object> perf) {
    for (LynxViewClient client : mClients) {
      client.onDynamicComponentPerfReady(perf);
    }
  }

  @Override
  public void onReportComponentInfo(Set<String> mComponentSet) {
    for (LynxViewClient client : mClients) {
      client.onReportComponentInfo(mComponentSet);
    }
  }

  @Override
  public void onDestroy() {
    TraceEvent.beginSection(TraceEventDef.CLIENT_DESTORY);
    for (LynxViewClient client : mClients) {
      client.onDestroy();
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_DESTORY);
  }

  @Override
  public String shouldRedirectImageUrl(String url) {
    for (LynxViewClient client : mClients) {
      String redirectUrl = client.shouldRedirectImageUrl(url);
      if (redirectUrl != null) {
        return redirectUrl;
      }
    }
    return null;
  }

  @Override
  public void loadImage(@NonNull Context context, @Nullable String cacheKey, @Nullable String src,
      float width, float height, final @Nullable Transformer transformer,
      @NonNull final CompletionHandler handler) {
    for (LynxViewClient client : mClients) {
      client.loadImage(context, cacheKey, src, width, height, transformer, handler);
    }
  }

  @Override
  public void onUpdateDataWithoutChange() {
    for (LynxViewClient client : mClients) {
      client.onUpdateDataWithoutChange();
    }
  }

  // issue: #1510
  @Override
  public void onModuleMethodInvoked(String module, String method, int error_code) {
    for (LynxViewClient client : mClients) {
      client.onModuleMethodInvoked(module, method, error_code);
    }
  }

  @Override
  public void onPiperInvoked(Map<String, Object> info) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_PIPER_INVOKED);
    for (LynxViewClient client : mClients) {
      client.onPiperInvoked(info);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_PIPER_INVOKED);
  }

  @Override
  public void onLynxViewAndJSRuntimeDestroy() {
    TraceEvent.beginSection(TraceEventDef.CLIENT_LYNXVIEW_AND_JSRUNTIME_DESTORY);
    for (LynxViewClient client : mClients) {
      client.onLynxViewAndJSRuntimeDestroy();
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_LYNXVIEW_AND_JSRUNTIME_DESTORY);
  }

  @Override
  public void onScrollStart(ScrollInfo info) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_SCROLL_START);
    for (LynxViewClient client : mClients) {
      client.onScrollStart(info);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_SCROLL_START);
  }

  @Override
  public void onScrollStop(ScrollInfo info) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_SCROLL_STOP);
    for (LynxViewClient client : mClients) {
      client.onScrollStop(info);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_SCROLL_STOP);
  }

  @Override
  public void onFling(ScrollInfo info) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_FLING);
    for (LynxViewClient client : mClients) {
      client.onFling(info);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_FLING);
  }

  @Override
  public void onFlushFinish(FlushInfo info) {
    for (LynxViewClient client : mClients) {
      client.onFlushFinish(info);
    }
  }

  @Override
  public void onKeyEvent(KeyEvent event, boolean handled) {
    for (LynxViewClient client : mClients) {
      client.onKeyEvent(event, handled);
    }
  }

  @Override
  public void onTimingUpdate(
      Map<String, Object> timingInfo, Map<String, Long> updateTiming, String flag) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_TIMING_UPDATE);
    for (LynxViewClient client : mClients) {
      client.onTimingUpdate(timingInfo, updateTiming, flag);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_TIMING_UPDATE);
  }

  @Override
  public void onTimingSetup(Map<String, Object> timingInfo) {
    TraceEvent.beginSection(TraceEventDef.CLIENT_TIMING_SETUP);
    for (LynxViewClient client : mClients) {
      client.onTimingSetup(timingInfo);
    }
    TraceEvent.endSection(TraceEventDef.CLIENT_TIMING_SETUP);
  }

  @Override
  public void onJSBInvoked(Map<String, Object> jsbInfo) {
    for (LynxViewClient client : mClients) {
      client.onJSBInvoked(jsbInfo);
    }
  }

  @Override
  public void onCallJSBFinished(Map<String, Object> jsbTiming) {
    for (LynxViewClient client : mClients) {
      client.onCallJSBFinished(jsbTiming);
    }
  }

  @Override
  public void onLynxEvent(LynxEventDetail detail) {
    for (LynxViewClient client : mClients) {
      client.onLynxEvent(detail);
    }
  }

  @Override
  public void onTemplateBundleReady(TemplateBundle bundle) {
    for (LynxViewClient client : mClients) {
      client.onTemplateBundleReady(bundle);
    }
  }
}
