// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.util.Set;

/**
 * Give the host application a chance to take control when a lynx template is about to be loaded in
 * the current LynxView.
 */
public class LynxViewClientV2 {
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
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {}
}
