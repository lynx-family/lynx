// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.lang.ref.WeakReference;

final class FramePerformanceClient extends LynxViewClientV2 {
  private static final String ENTRY_TYPE_PIPELINE = "pipeline";
  private static final String ENTRY_NAME_LOAD_BUNDLE = "loadBundle";

  private final WeakReference<LynxFrameView> mFrameViewRef;

  FramePerformanceClient(@NonNull LynxFrameView frameView) {
    mFrameViewRef = new WeakReference<>(frameView);
  }

  static boolean shouldHandlePerformanceEntry(PerformanceEntry entry) {
    return entry != null && ENTRY_TYPE_PIPELINE.equals(entry.entryType)
        && ENTRY_NAME_LOAD_BUNDLE.equals(entry.name);
  }

  @Override
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
    LynxFrameView frameView = mFrameViewRef.get();
    if (frameView == null || !shouldHandlePerformanceEntry(entry)) {
      return;
    }
    frameView.onFrameLoadMetricsEvent(entry);
  }
}
