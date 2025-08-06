// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import android.graphics.Rect;
import androidx.annotation.RestrictTo;

/**
 * Information about content for FSP tracing.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPContentInfo {
  private final int uiSign;
  private final long changeTimestampNs;
  private final Rect rect;

  public FSPContentInfo(int uiSign, long changeTimestampNs, Rect rect) {
    this.uiSign = uiSign;
    this.changeTimestampNs = changeTimestampNs;
    this.rect = rect;
  }

  public int getUiSign() {
    return uiSign;
  }

  public long getChangeTimestampNs() {
    return changeTimestampNs;
  }

  public Rect getRect() {
    return rect;
  }
}
