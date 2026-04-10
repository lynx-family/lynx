// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import android.os.Build;
import android.view.Choreographer;
import androidx.annotation.RequiresApi;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;

@RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
public class ChoreographerLayoutTick implements LayoutTick {
  private final LynxContext mLynxContext;

  public ChoreographerLayoutTick(LynxContext context) {
    mLynxContext = context;
  }
  @Override
  public void request(final Runnable runnable) {
    if (runnable == null) {
      return;
    }
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.beginSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_REQUEST_LAYOUT);
    }
    Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        if (mLynxContext.hasLayoutThreadChanged()) {
          mLynxContext.runOnLayoutThread(new Runnable() {
            @Override
            public void run() {
              if (TraceEvent.isTracingStarted()) {
                TraceEvent.beginSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_TRIGGER_LAYOUT);
              }
              runnable.run();
              if (TraceEvent.isTracingStarted()) {
                TraceEvent.endSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_TRIGGER_LAYOUT);
              }
            }
          });
        } else {
          if (TraceEvent.isTracingStarted()) {
            TraceEvent.beginSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_TRIGGER_LAYOUT);
          }
          runnable.run();
          if (TraceEvent.isTracingStarted()) {
            TraceEvent.endSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_TRIGGER_LAYOUT);
          }
        }
      }
    });
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(TraceEventDef.CHOREOGRAPHER_LAYOUT_TICK_REQUEST_LAYOUT);
    }
  }
}
