// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import android.content.Context;
import android.view.Choreographer;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.UnitUtils;

public abstract class UIListAutoScroller {
  private boolean mStart = false;
  protected int mAutoRatePerFrame = 0;
  private boolean mAutoStopOnBounds = true;
  private Choreographer.FrameCallback mFrameCallback = null;

  abstract void onAutoScrollError(String msg);

  abstract void onAutoScrollStart();

  abstract void onAutoScrollEnd();

  // TODO(xiamengfei.moonface) Abstract the interfaces of canScroll and scrollBy for a scrollable
  // container.
  abstract boolean canScroll(int distance);

  abstract void scrollBy(int distance);

  public void setAutoScrollParams(boolean start, boolean autoStopOnBounds) {
    mStart = start;
    mAutoStopOnBounds = autoStopOnBounds;
  }

  public void execute(String ratePerSecond, Context context) {
    if (mStart) {
      int px = (int) UnitUtils.toPx(ratePerSecond, 0);
      if (px == 0) {
        onAutoScrollError("rate is not right");
        return;
      }
      int refreshRate = (int) DeviceUtils.getRefreshRate(context);
      // prevent 0 from being a divisor
      if (refreshRate <= 0) {
        refreshRate = DeviceUtils.DEFAULT_DEVICE_REFRESH_RATE;
      }
      // if list's scrolling direction is up, mAutoRatePerFrame >0; otherwise,mAutoRatePerFrame <0;
      mAutoRatePerFrame = px > 0 ? Math.max(px / refreshRate, 1) : Math.min(px / refreshRate, -1);
      // stop last scroll by removing FrameCallback
      removeFrameCallback();
      onAutoScrollStart();

      autoScroll();
    } else {
      onAutoScrollEnd();
      removeFrameCallback();
    }
  }

  private void autoScroll() {
    mFrameCallback = new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        // Check if this view can be scrolled vertically/horizontally in a certain direction.
        // direction:Negative to check scrolling up, positive to check scrolling down.
        boolean canScroll = canScroll(mAutoRatePerFrame);
        if (canScroll) {
          scrollBy(mAutoRatePerFrame);
        }
        // should post FrameCallback on two scenes:
        // firstly: the switch of "enableAutoScroll" is true and the list  can be Scrolled.
        // secondly:the switch of "enableAutoScroll" is true and mAutoStopOnBounds is
        // false.mAutoStopOnBounds is false, it means that the list can not stop auto scrolling even
        // if it comes to the boarder
        if (mStart && (canScroll || !mAutoStopOnBounds)) {
          if (mFrameCallback != null) {
            Choreographer.getInstance().postFrameCallback(mFrameCallback);
          }
        } else {
          if (!mStart || !canScroll) {
            onAutoScrollEnd();
          }
          // remove FrameCallback
          removeFrameCallback();
        }
      }
    };
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }

  public void removeFrameCallback() {
    if (mFrameCallback != null) {
      Choreographer.getInstance().removeFrameCallback(mFrameCallback);
      mFrameCallback = null;
    }
  }
}
