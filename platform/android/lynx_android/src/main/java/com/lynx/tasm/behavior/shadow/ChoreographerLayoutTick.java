// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import android.os.Build;
import android.view.Choreographer;
import androidx.annotation.RequiresApi;

@RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
public class ChoreographerLayoutTick implements LayoutTick {
  @Override
  public void request(final Runnable runnable) {
    if (runnable == null) {
      return;
    }
    Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        runnable.run();
      }
    });
  }
}
