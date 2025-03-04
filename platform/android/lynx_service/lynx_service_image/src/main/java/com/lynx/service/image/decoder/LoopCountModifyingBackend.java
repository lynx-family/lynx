// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.image.decoder;

import com.facebook.fresco.animation.backend.AnimationBackend;
import com.facebook.fresco.animation.backend.AnimationBackendDelegate;

public class LoopCountModifyingBackend extends AnimationBackendDelegate {
  private int mLoopCount;

  public LoopCountModifyingBackend(AnimationBackend animationBackend, int loopCount) {
    super(animationBackend);
    mLoopCount = loopCount;
  }

  @Override
  public int getLoopCount() {
    return mLoopCount;
  }
}
