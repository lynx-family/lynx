// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import androidx.annotation.GuardedBy;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import java.lang.Runnable;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class LynxLayoutProxy {
  // ensure thread safe
  @GuardedBy("mLock") private long mNativePtr;
  private final ReadWriteLock mLock = new ReentrantReadWriteLock();
  private final static String TAG = "LynxLayoutProxy";

  public LynxLayoutProxy(long lynxShellPtr) {
    initNative(lynxShellPtr);
    LLog.i(TAG, "LynxLayoutProxy is created");
  }

  protected void initNative(long lynxShellPtr) {
    mNativePtr = nativeCreate(lynxShellPtr);
  }

  public void runOnLayoutThread(@NonNull Runnable runnable) {
    mLock.readLock().lock();
    if (mNativePtr != 0) {
      LLog.i(TAG, "runOnLayoutThread is invoked");
      nativeRunOnLayoutThread(mNativePtr, runnable);
    }
    mLock.readLock().unlock();
  }

  public void destroy() {
    mLock.writeLock().lock();
    if (mNativePtr != 0) {
      LLog.i(TAG, "LynxLayoutProxy will be destroyed");
      nativeRelease(mNativePtr);
      mNativePtr = 0;
    }
    mLock.writeLock().unlock();
  }

  private native long nativeCreate(long lynxShellPtr);

  private native void nativeRelease(long nativePtr);

  private native void nativeRunOnLayoutThread(long nativePtr, Runnable runnable);
}
