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

/**
 * @apidoc
 * @brief A client that provides callbacks for `LynxView`'s lifecycle and other events.
 */
public abstract class LynxViewClient
    extends LynxBackgroundRuntimeClient implements ImageInterceptor {
  // issue: #1510
  /**
   * @apidoc
   * @brief Called when module method invocation completed.
   * @param module Module name.
   * @param method Method name.
   * @param error_code Error code if module method invocation failed.
   */
  public void onModuleMethodInvoked(String module, String method, int error_code) {}

  /**
   * @apidoc
   * @brief Called when page start loading.
   * @param url The page URL
   */
  public void onPageStart(@Nullable String url) {}

  /**
   * @apidoc
   * @brief Called when page load finish.
   */
  public void onLoadSuccess() {}

  /**
   * Report core metrics after the page loads successfully
   */
  @Deprecated
  public void onReportLynxConfigInfo(LynxConfigInfo info) {}

  /**
   * @apidoc
   * @brief Called when first screen layout completed.
   */
  public void onFirstScreen() {}

  /**
   * @apidoc
   * @brief Called when page update.
   */
  public void onPageUpdate() {}

  /**
   * @apidoc
   * @brief Called when data update, but the view may not be updated.
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
   * @apidoc
   * @brief Called when JS environment preparation completed.
   * @note The callback is in an asynchronous thread.
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
   * @apidoc
   * @brief Called when error received.
   * @param error The type and information of the error.
   */
  public void onReceivedError(LynxError error) {}

  /**
   * @apidoc
   * @brief Called when java error received.
   * @param error The type and information of the error.
   */
  public void onReceivedJavaError(LynxError error) {}

  /**
   * @apidoc
   * @brief Called when JS error received.
   * @param jsError JS exception information.
   */
  public void onReceivedJSError(LynxError jsError) {}

  /**
   * @apidoc
   * @brief Called when C++ layer error received.
   * @param nativeError Native exception information.
   */
  public void onReceivedNativeError(LynxError nativeError) {}

  /**
   * @apidoc
   * @brief Performance data statistics callback after the first load is completed.
   * @note The timing of the callback is not fixed due to differences in rendering threads, and
   * should not be used as a starting point for any business side. The callback is in the main
   * thread.
   */
  public void onFirstLoadPerfReady(LynxPerfMetric metric) {}

  /**
   * @apidoc
   * @brief Performance data statistics callback after the interface update is completed.
   * @note The timing of the callback is not fixed due to differences in rendering threads, and
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
   * @apidoc
   * @brief Return the used component tagName.
   * @param mComponentSet The set of component tagName.
   */
  public void onReportComponentInfo(Set<String> mComponentSet) {}

  /**
   * @apidoc
   * @brief Called after the page is destroyed.
   * @note The callback executed in the main thread.
   */
  public void onDestroy() {}

  /**
   * @apidoc
   * @brief If there is no need to update the UI after calling
   * [`updateData`](../lynx-view/update-meta-data.mdx), this method is called back.
   */
  public void onUpdateDataWithoutChange() {}

  /**
   * @apidoc
   * @brief Called when the UI start scrolling.
   * @param info Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name.
   */
  public void onScrollStart(ScrollInfo info) {}

  /**
   * @apidoc
   * @brief Called when the UI finished scrolling.
   * @param info Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name.
   */
  public void onScrollStop(ScrollInfo info) {}

  /**
   * @apidoc
   * @brief Called when inertial scrolling starts after releasing the finger.
   * @param info Contains scroll parameters, including the instance of the scrolling view, and
   * the front-end specified id and name
   */
  public void onFling(ScrollInfo info) {}

  /**
   * @apidoc
   * @brief Called when UI flush end in async render mode.
   * @param flushInfo Contains the begin and end timings of the flush, and whether it is a
   * synchronous flush.
   */
  public void onFlushFinish(FlushInfo flushInfo) {}

  /**
   * @apidoc
   * @brief Called when native layout finish in ui or most_on_tasm mode, or diff finish in
   * multi_thread mode.
   */
  public void onTASMFinishedByNative() {}

  /**
   * @apidoc
   * @brief Called when JSBridge invoked.
   * @param info JSBridge's information. `url: String`: Lynx's url; `module-name: String`:
   * module's name; `method-name: String`: method's name; `params: List<Object>`: (Optional) other
   * necessary parameters.
   */
  public void onPiperInvoked(Map<String, Object> info) {}

  /**
   * @apidoc
   * @brief Called when lynx view and js runtime destroy.
   */
  public void onLynxViewAndJSRuntimeDestroy() {}

  /**
   * @apidoc
   * @brief Report all android key events with flag indicating whether has been handled.
   * @param event The android key.
   * @param handled Indicate whether the event has been handled(consumed) by lynx.
   */
  public void onKeyEvent(KeyEvent event, boolean handled) {}

  public void onTimingSetup(Map<String, Object> timingInfo) {}

  public void onTimingUpdate(
      Map<String, Object> timingInfo, Map<String, Long> updateTiming, String flag) {}

  public void onJSBInvoked(Map<String, Object> jsbInfo) {}

  public void onCallJSBFinished(Map<String, Object> jsbTiming) {}

  /**
   * @apidoc
   * @brief Report Lynx events that sended to frontend.
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
  public @Nullable String shouldRedirectImageUrl(String url) {
    return null;
  }

  public void loadImage(@NonNull Context context, @Nullable String cacheKey, @Nullable String src,
      float width, float height, final @Nullable Transformer transformer,
      @NonNull final CompletionHandler handler) {}

  /**
   * @apidoc
   * @brief Provide a reusable TemplateBundle after template is decode.
   * @note This callback is disabled by default, and you can enable it through the
   * DUMP_ELEMENT option or RECYCLE_TEMPLATE_BUNDLE option in LynxLoadMeta.
   * @param bundle The recycled template bundle, it is nonnull but could be invalid.
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
