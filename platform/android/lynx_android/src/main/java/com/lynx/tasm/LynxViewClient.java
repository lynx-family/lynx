// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.view.KeyEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ImageInterceptor;
import com.lynx.tasm.event.LynxEventDetail;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import javax.xml.transform.Transformer;

public abstract class LynxViewClient
    extends LynxBackgroundRuntimeClient implements ImageInterceptor {
  // issue: #1510
  /**
   * Module Method invocation completed
   * @param module The name of the module
   * @param method The name of the method
   * @param error_code The error code
   */
  public void onModuleMethodInvoked(String module, String method, int error_code) {}

  /**
   * Page starts preparing to load
   * @param url The page URL
   */
  public void onPageStart(@Nullable String url) {}

  /**
   * Page loaded successfully
   */
  public void onLoadSuccess() {}

  /**
   * Report core metrics after the page loads successfully
   */
  @Deprecated
  public void onReportLynxConfigInfo(LynxConfigInfo info) {}

  /**
   * First screen layout completed
   */
  public void onFirstScreen() {}

  /**
   * Page update callback
   */
  public void onPageUpdate() {}

  /**
   * Data update callback
   */
  public void onDataUpdated() {}
  /**
   * Load failed
   * @param message
   * Use {@link #onReceivedError(LynxError)} instead
   */
  @Deprecated
  public void onLoadFailed(String message) {}

  /**
   * JS environment preparation completed
   * Note: The callback is in an asynchronous thread.
   */
  public void onRuntimeReady() {}

  /**
   * Error received
   * @param info
   * Use {@link #onReceivedError(LynxError)} instead
   */
  @Deprecated
  public void onReceivedError(String info) {}

  /**
   * Error received
   * @param error The type and information of the error
   */
  public void onReceivedError(LynxError error) {}

  /**
   * Java error received
   * @param error The type and information of the error
   */
  public void onReceivedJavaError(LynxError error) {}

  /**
   * JS error received
   * @param jsError JS exception information
   */
  public void onReceivedJSError(LynxError jsError) {}

  /**
   * C++ layer error received
   * @param nativeError Native exception information
   */
  public void onReceivedNativeError(LynxError nativeError) {}

  /**
   * Performance data statistics callback after the first load is completed.
   * NOTE: The timing of the callback is not fixed due to differences in rendering threads, and
   * should not be used as a starting point for any business side. The callback is in the main
   * thread.
   */
  public void onFirstLoadPerfReady(LynxPerfMetric metric) {}

  /**
   * Performance data statistics callback after the interface update is completed.
   * NOTE: The timing of the callback is not fixed due to differences in rendering threads, and
   * should not be used as a starting point for any business side. The callback is in the main
   * thread.
   */
  public void onUpdatePerfReady(LynxPerfMetric metric) {}

  /**
   * Performance data statistics callback after the first load or interface update is completed for
   * dynamic components. NOTE: The timing of the callback is not fixed due to differences in
   * rendering threads, and should not be used as a starting point for any business side. The
   * callback is in the main thread.
   */
  @Deprecated
  public void onDynamicComponentPerfReady(HashMap<String, Object> perf) {}

  /**
   * Return the used component tagName
   * @param mComponentSet immutableSet
   */
  public void onReportComponentInfo(Set<String> mComponentSet) {}

  /**
   * Callback after the page is destroyed
   * NOTE: The callback is in the main thread
   */
  public void onDestroy() {}

  /**
   * If there is no need to update the UI after calling {@link LynxView#updateData}, this method is
   * called back
   */
  public void onUpdateDataWithoutChange() {}

  /**
   * When the UI starts scrolling, this method is called back
   * @param ScrollInfo Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name
   */
  public void onScrollStart(ScrollInfo info) {}

  /**
   * When the UI finishes scrolling, this method is called back
   * @param ScrollInfo Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name
   */
  public void onScrollStop(ScrollInfo info) {}

  /**
   * Callback when inertial scrolling starts after releasing the finger
   * @param ScrollInfo Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name
   */
  public void onFling(ScrollInfo info) {}

  /**
   * for async render, flush end will call this
   *
   * {@link com.lynx.tasm.behavior.operations.queue.UIOperationQueueAsyncRender}
   *
   * @param flushInfo include begin timing, end timing and is sync flush or not
   */
  public void onFlushFinish(FlushInfo flushInfo) {}

  /**
   * on mode of ui/most_on_tasm, it will be triggered on layout finish by native
   * on mode of multi_thread, it will be triggered on diff finish by native
   */
  public void onTASMFinishedByNative() {}

  /**
   * @param info JSBridge's information
   *             "url" : (String)Lynx's url
   *             "module-name" (String): module's name
   *             "method-name" (String): method's name
   *             "params" (List<Object>): (Optional) other necessary parameters
   */
  public void onPiperInvoked(Map<String, Object> info) {}

  public void onLynxViewAndJSRuntimeDestroy() {}

  /**
   * Report all android key events with flag indicating whether has been handeld.
   * @param event the android key.
   * @param handled indicate whether the event has been handled(consumed) by lynx.
   */
  public void onKeyEvent(KeyEvent event, boolean handled) {}

  public void onTimingSetup(Map<String, Object> timingInfo) {}

  public void onTimingUpdate(
      Map<String, Object> timingInfo, Map<String, Long> updateTiming, String flag) {}

  public void onJSBInvoked(Map<String, Object> jsbInfo) {}

  public void onCallJSBFinished(Map<String, Object> jsbTiming) {}

  /**
   * Report Lynx events that sended to frontend.
   *
   * @param detail LynxEvent that will send to frontend, including eventName, lynxview, etc.
   */
  public void onLynxEvent(LynxEventDetail detail) {}

  /**
   * Deprecated, use {@link ImageInterceptor} instead <p>
   *
   * Notify the host application of a image request and allow the application
   * to redirect the url and return. If the return value is null, LynxView will
   * continue to load image from the origin url as usual. Otherwise, the redirect
   * url will be used.
   *
   * This method will be called on any thread.
   *
   * The following scheme is supported in LynxView:
   * 1. Http scheme: http:// or https://
   * 2. File scheme: file:// + path
   * 3. Assets scheme: asset:///
   * 4. Res scheme: res:///identifier or res:///image_name
   *
   * @param url the url that ready for loading image
   * @return A url string that fit int with the support scheme list or null
   */
  @Deprecated
  public String shouldRedirectImageUrl(String url) {
    return null;
  }

  public void loadImage(@NonNull Context context, @Nullable String cacheKey, @Nullable String src,
      float width, float height, final @Nullable Transformer transformer,
      @NonNull final CompletionHandler handler) {}

  /**
   * Provide a reusable TemplateBundle after template is decode.
   * NOTE: This callback is disabled by default, and you can enable it through the
   * DUMP_ELEMENT option or RECYCLE_TEMPLATE_BUNDLE option in LynxLoadMeta.
   * @param bundle the recycled template bundle, it is nonnull but could be invalid
   */
  public void onTemplateBundleReady(@NonNull TemplateBundle bundle) {}

  public static class ScrollInfo {
    public View mView;
    public String mTagName;
    public String mScrollMonitorTag;

    public ScrollInfo(View view, String tagName, String scrollMonitorTag) {
      mView = view;
      mTagName = tagName;
      mScrollMonitorTag = scrollMonitorTag;
    }

    @Override
    public String toString() {
      return String.format("ViewInfo @%d view %s, name %s, monitor-name %s", hashCode(),
          mView.getClass().getSimpleName(), mTagName, mScrollMonitorTag);
    }
  }

  public static class FlushInfo {
    public final boolean syncFlush;
    public final long beginTiming;
    public final long endTiming;

    public FlushInfo(final boolean syncFlush, long beginTiming, long endTiming) {
      this.syncFlush = syncFlush;
      this.beginTiming = beginTiming;
      this.endTiming = endTiming;
    }

    @Override
    public String toString() {
      return String.format("FlushInfo is sync:" + syncFlush + ", begin timing:" + beginTiming
          + ", end timing:" + endTiming);
    }
  }
}
