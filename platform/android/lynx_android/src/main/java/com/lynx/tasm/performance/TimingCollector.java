// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import androidx.annotation.GuardedBy;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.base.CalledByNative;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * `TimingCollector` is a utility class designed to measure the time spent
 * on rendering operations within the platform layers, such as create_lynxview & draw.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class TimingCollector {
  private final ReadWriteLock mLock = new ReentrantReadWriteLock();
  @GuardedBy("mLock") private long mNativeTimingCollectorPtr = 0;

  public TimingCollector() {}

  public void init() {
    mLock.writeLock().lock();
    try {
      if (mNativeTimingCollectorPtr == 0) {
        mNativeTimingCollectorPtr = nativeCreateTimingCollector();
      }
    } finally {
      mLock.writeLock().unlock();
    }
  }

  public long getNativeTimingCollectorPtr() {
    mLock.readLock().lock();
    long ptr = mNativeTimingCollectorPtr;
    mLock.readLock().unlock();
    return ptr;
  }

  /**
   * It's necessary to invoke destroy method to avoid mem leak.
   */
  public void destroy() {
    mLock.writeLock().lock();
    try {
      if (mNativeTimingCollectorPtr != 0) {
        nativeReleaseTimingCollector(mNativeTimingCollectorPtr);
        mNativeTimingCollectorPtr = 0;
      }
    } finally {
      mLock.writeLock().unlock();
    }
  }

  public void markDrawEndTimingIfNeeded() {
    mLock.readLock().lock();
    try {
      if (mNativeTimingCollectorPtr != 0) {
        nativeMarkDrawEndTimingIfNeeded(mNativeTimingCollectorPtr);
      }
    } finally {
      mLock.readLock().unlock();
    }
  }

  public void setMsTiming(final String key, final long msTimestamp, final String pipelineID) {
    setTiming(key, msTimestamp * 1000, pipelineID);
  }

  private void setTiming(String key, long timestamp, String pipelineID) {
    mLock.readLock().lock();
    try {
      if (mNativeTimingCollectorPtr != 0) {
        nativeSetTiming(mNativeTimingCollectorPtr, pipelineID, key, timestamp);
      }
    } finally {
      mLock.readLock().unlock();
    }
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (extraTiming.mOpenTime > 0) {
      setMsTiming(TimingHandler.OPEN_TIME, extraTiming.mOpenTime, null);
    }
    if (extraTiming.mContainerInitStart > 0) {
      setMsTiming(TimingHandler.CONTAINER_INIT_START, extraTiming.mContainerInitStart, null);
    }
    if (extraTiming.mContainerInitEnd > 0) {
      setMsTiming(TimingHandler.CONTAINER_INIT_END, extraTiming.mContainerInitEnd, null);
    }
    if (extraTiming.mPrepareTemplateStart > 0) {
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_START, extraTiming.mPrepareTemplateStart, null);
    }
    if (extraTiming.mPrepareTemplateEnd > 0) {
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, extraTiming.mPrepareTemplateEnd, null);
    }
  }

  private native long nativeCreateTimingCollector();

  private native void nativeReleaseTimingCollector(long nativePtr);
  private native void nativeMarkDrawEndTimingIfNeeded(long nativePtr);

  private native void nativeSetTiming(
      long nativePtr, String pipelineID, String timingKey, long timestamp);
}
