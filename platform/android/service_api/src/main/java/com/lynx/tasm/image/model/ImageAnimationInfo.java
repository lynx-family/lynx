// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;

public final class ImageAnimationInfo {
  private final int mFrameCount;
  private final long mLoopDurationMs;

  public ImageAnimationInfo(int frameCount, long loopDurationMs) {
    mFrameCount = frameCount;
    mLoopDurationMs = loopDurationMs;
  }

  public int getFrameCount() {
    return mFrameCount;
  }

  public long getLoopDurationMs() {
    return mLoopDurationMs;
  }
}
