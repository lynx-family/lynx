// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class PerformanceController {
  private volatile long mNativePerformanceActorPtr = 0;
  private volatile boolean mIsNativeLibraryLoaded = false;
  private IPerformanceObserver mObserver;

  public void setPerformanceObserver(IPerformanceObserver observer) {
    mObserver = observer;
  }

  public void init() {
    if (!mIsNativeLibraryLoaded) {
      mIsNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    }
    if (mIsNativeLibraryLoaded) {
      mNativePerformanceActorPtr = nativeCreateNativePerformanceActorPtr();
    }
  }
  /**
   * It's necessary to invoke destroy method to avoid mem leak.
   */
  public void destroy() {
    long lastNativePerformanceActorPtr = mNativePerformanceActorPtr;
    mNativePerformanceActorPtr = 0;
    if (lastNativePerformanceActorPtr != 0) {
      nativeReleaseNativePerformanceActorPtr(lastNativePerformanceActorPtr);
    }
  }

  public void allocateMemory(String category, float sizeKb, String detailKey, String detailValue) {
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    nativeAllocateMemory(mNativePerformanceActorPtr, category, sizeKb, detailKey, detailValue);
  }

  public void deallocateMemory(
      String category, float sizeKb, String detailKey, String detailValue) {
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    nativeDeallocateMemory(mNativePerformanceActorPtr, category, sizeKb, detailKey, detailValue);
  }

  public void updateMemoryUsage(
      String category, float sizeKb, String detailKey, String detailValue) {
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    nativeUpdateMemoryUsage(mNativePerformanceActorPtr, category, sizeKb, detailKey, detailValue);
  }

  @CalledByNative
  protected long getNativePtr() {
    return mNativePerformanceActorPtr;
  }

  @CalledByNative
  protected void onPerformanceEvent(ReadableMap entryMap) {
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (mObserver != null) {
      mObserver.onPerformanceEvent(entry);
    }
  }

  private native long nativeCreateNativePerformanceActorPtr();
  private native void nativeReleaseNativePerformanceActorPtr(long nativePtr);
  private native void nativeAllocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);

  private native void nativeDeallocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);

  private native void nativeUpdateMemoryUsage(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeSetForceEnable(long nativePtr, boolean forceEnable);
}
