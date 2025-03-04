// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.tracing;

import com.lynx.BuildConfig;
import com.lynx.tasm.base.TraceEvent;

public class FrameTraceService {
  private long mNativePtr = 0;

  private FrameTraceService() {
    mNativePtr = nativeCreateFrameTraceService();
  }

  private static class FrameTraceServiceLoader {
    private static final FrameTraceService INSTANCE = new FrameTraceService();
  }

  public static FrameTraceService getInstance() {
    return FrameTraceServiceLoader.INSTANCE;
  }

  public void initializeService() {
    nativeInitialize(mNativePtr);
  }

  public void FPSTrace(long startTime, long endTime) {
    if (TraceEvent.enablePerfettoTrace()) {
      nativeFPSTrace(mNativePtr, startTime, endTime);
    }
  }

  public void screenshot(String snapshot) {
    if (TraceEvent.enablePerfettoTrace()
        && TraceEvent.categoryEnabled(TraceEvent.CATEGORY_SCREENSHOTS)) {
      nativeScreenshot(mNativePtr, snapshot);
    }
  }

  private native void nativeInitialize(long nativePtr);
  private native long nativeCreateFrameTraceService();
  private native void nativeFPSTrace(long nativePtr, long startTime, long endTime);
  private native void nativeScreenshot(long nativePtr, String snapshot);
}
