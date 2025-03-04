// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.tasm.common.SingleThreadAsserter;

public class CustomLayoutTick implements LayoutTick {
  private Runnable mRunnable;
  private boolean mWaitForLayout = false;
  private android.os.Handler mLayoutTriggerHandler = null;
  private static final int mNextTickDelayMillis = 16;
  private SingleThreadAsserter mSingleThreadAsserter;

  public void request(Runnable runnable) {
    if (mLayoutTriggerHandler == null) {
      mLayoutTriggerHandler = new android.os.Handler();
      mSingleThreadAsserter = new SingleThreadAsserter();
    }
    mSingleThreadAsserter.assertNow();

    if (mWaitForLayout)
      return;

    mRunnable = runnable;
    mLayoutTriggerHandler.postDelayed(new Runnable() {
      @Override
      public void run() {
        triggerLayout();
        mWaitForLayout = false;
      }
    }, mNextTickDelayMillis);

    mWaitForLayout = true;
  }

  private void triggerLayout() {
    if (mRunnable != null) {
      mRunnable.run();
    }
  }
}
