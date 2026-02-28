// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import java.util.Set;

/**
 * Give the host application a chance to take control when a lynx template is about to be loaded in
 * the current LynxView.
 */
public class LynxViewClientV2 implements IPerformanceObserver {
  /**
   * Provide information about the lynx pixel pipeline
   */
  public static class LynxPipelineInfo {
    /**
     * The cause that the lynx pixel pipeline is activated
     */
    public enum LynxPipelineOrigin {
      LYNX_FIRST_SCREEN(1),
      LYNX_RELOAD(1 << 1);

      private final int origin;

      LynxPipelineOrigin(int origin) {
        this.origin = origin;
      }

      int getValue() {
        return origin;
      }
    }

    private final String url;
    private int pipelineOrigin;

    public LynxPipelineInfo(String url) {
      this.url = url;
    }

    /**
     * @return url of LynxView
     */
    public String getUrl() {
      return url;
    }

    /**
     * @return whether the pixel pipeline is caused by first screen
     */
    public boolean isFromFirstScreen() {
      return (this.pipelineOrigin & LynxPipelineOrigin.LYNX_FIRST_SCREEN.getValue()) > 0;
    }

    /**
     * @return whether the pixel pipeline is caused by reload
     */
    public boolean isFromReload() {
      return (this.pipelineOrigin & LynxPipelineOrigin.LYNX_RELOAD.getValue()) > 0;
    }

    public void addPipelineOrigin(LynxPipelineOrigin pipelineOrigin) {
      this.pipelineOrigin |= pipelineOrigin.getValue();
    }
  }

  /**
   * Notify that a lynx template has started loading. It will be call at both `loadTemplate` and
   * `reloadTemplate`.
   *
   * Note: this method will be executed before the main process of lynx so do not execute overly
   * complex logic in this method.
   *
   * @param lynxView the LynxView which has started loading
   * @param info the information about the pixel pipeline
   *
   */
  public void onPageStarted(@Nullable LynxView lynxView, @NonNull LynxPipelineInfo info) {}

  /**
   * Notify the client that a performance event has been sent. It will be called every time a
   * performance event is generated, including but not limited to container initialization, engine
   * rendering, rendering metrics update, etc.
   *
   * Note: This method is for performance events and will be executed on the reporter thread, so do
   * not execute complex logic or UI modification logic in this method.
   *
   * @param entry the PerformanceEntry about the performance event
   *
   */
  @Override
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {}

  /**
   * Provide information about the loaded resource.
   */
  public static class LynxResourceLoadInfo {
    private final LynxResourceRequest.LynxResourceType mResourceType;
    private final int mErrCode;
    @Nullable private final String mErrMsg;

    /**
     * Create a new LynxResourceLoadInfo.
     *
     * @param type the type of the resource
     * @param errCode the error code of the load result, 0 means success
     * @param errMsg the error message of the load result, if any
     */
    public LynxResourceLoadInfo(
        LynxResourceRequest.LynxResourceType type, int errCode, @Nullable String errMsg) {
      mResourceType = type;
      mErrCode = errCode;
      mErrMsg = errMsg;
    }

    /**
     * @return the type of the resource
     */
    public LynxResourceRequest.LynxResourceType getResourceType() {
      return mResourceType;
    }

    /**
     * @return the error code of the load result, 0 means success
     */
    public int getErrCode() {
      return mErrCode;
    }

    /**
     * @return the error message of the load result, or {@code null} if not available
     */
    @Nullable
    public String getErrMsg() {
      return mErrMsg;
    }
  }

  /**
   * Notify the client that a resource has finished loading.
   *
   * Note: This method is for performance events and will be executed on the reporter thread, so do
   * not execute complex logic or UI modification logic in this method.
   *
   * @param info information about the resource load result
   */
  public void onResourceLoaded(@NonNull LynxResourceLoadInfo info) {}
}
