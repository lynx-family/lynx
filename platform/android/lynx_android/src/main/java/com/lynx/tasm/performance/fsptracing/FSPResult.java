// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import android.os.SystemClock;
import androidx.annotation.RestrictTo;
import java.lang.System;

/**
 * Result of FSP tracing.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPResult {
  public static final int UNKNOWN_UI_SIGN = -1;

  private boolean success;
  private final int contentWidth;
  private final int contentHeight;
  private final int lastUISign;
  private final long lastChangeTimestampNs;

  public FSPResult() {
    this(false, UNKNOWN_UI_SIGN, 0, 0, 0);
  }

  public FSPResult(boolean success, int lastUISign, long lastChangeTimestampNs, int contentWidth,
      int contentHeight) {
    this.success = success;
    this.lastUISign = lastUISign;
    this.lastChangeTimestampNs = lastChangeTimestampNs;
    this.contentWidth = contentWidth;
    this.contentHeight = contentHeight;
  }

  public boolean isSuccess() {
    return success;
  }

  public void setSuccess(boolean success) {
    this.success = success;
  }

  public int getLastUISign() {
    return lastUISign;
  }

  public long getLastChangeTimestampNs() {
    return lastChangeTimestampNs;
  }

  public long getLastChangeTimestampMs() {
    long nowNs = SystemClock.elapsedRealtimeNanos();
    long nowMs = System.currentTimeMillis();
    long diffMs = (lastChangeTimestampNs - nowNs) / 1000_000;
    long fspMs = nowMs + diffMs;
    return fspMs;
  }

  public int getContentWidth() {
    return contentWidth;
  }

  public int getContentHeight() {
    return contentHeight;
  }
}
