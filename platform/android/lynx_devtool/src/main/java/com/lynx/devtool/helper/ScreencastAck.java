// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import java.util.concurrent.atomic.AtomicInteger;

/** One outstanding frame per session. An old ACK must never advance the reference image. */
final class ScreencastAck {
  private static final AtomicInteger NEXT_ID = new AtomicInteger();
  private int mPending;
  private int mReference;
  private long mSentAt;
  private int mDeltaCount;

  int send(long now, boolean delta) {
    if (mPending != 0) {
      throw new IllegalStateException("A screencast frame is already pending");
    }
    int id = NEXT_ID.incrementAndGet();
    if (id == 0) {
      id = NEXT_ID.incrementAndGet();
    }
    mPending = id;
    mSentAt = now;
    mDeltaCount = delta ? mDeltaCount + 1 : 0;
    return id;
  }

  boolean acknowledge(int id, boolean applied) {
    if (id == 0 || id != mPending) {
      return false;
    }
    mReference = applied ? id : 0;
    mPending = 0;
    return true;
  }

  boolean expire(long now) {
    if (mPending == 0 || now - mSentAt < 2000) {
      return false;
    }
    reset();
    return true;
  }

  void reset() {
    mPending = 0;
    mReference = 0;
    mDeltaCount = 0;
  }

  boolean isPending() {
    return mPending != 0;
  }

  int reference() {
    return mReference;
  }

  boolean needsFullFrame() {
    return mReference == 0 || mDeltaCount >= 60;
  }
}
