// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.RuntimeLifecycleListener;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.utils.UIThreadUtils;

/** Owns the native Host Script Session for one debug Host Script Runtime. */
@Keep
@RestrictTo(RestrictTo.Scope.LIBRARY)
final class LynxHostScriptRuntimeBinding implements RuntimeLifecycleListener {
  interface EntryResultListener {
    void onEntryResult(String status, @Nullable String message);
  }

  private final EntryResultListener mEntryResultListener;
  private volatile boolean mRuntimeDetached;
  private long mNativePtr;

  LynxHostScriptRuntimeBinding(EntryResultListener listener) {
    mEntryResultListener = listener;
    mNativePtr = nativeCreate(this);
    if (mNativePtr == 0) {
      throw new IllegalStateException("Cannot create Host Script Session");
    }
  }

  @Override
  public synchronized void onRuntimeAttach(long napiEnv, String runtimeType) {
    long nativePtr = nativePtr();
    if (nativePtr != 0 && napiEnv != 0) {
      mRuntimeDetached = false;
      nativeOnRuntimeAttach(nativePtr, napiEnv);
    }
  }

  @Override
  public synchronized void onRuntimeDetach() {
    long nativePtr = nativePtr();
    mRuntimeDetached = true;
    if (nativePtr != 0) {
      nativeOnRuntimeDetach(nativePtr);
    }
  }

  synchronized long nativePtr() {
    return mNativePtr;
  }

  synchronized long activeNativePtr() {
    return mRuntimeDetached ? 0 : mNativePtr;
  }

  synchronized void destroy() {
    if (mNativePtr == 0) {
      return;
    }
    long nativePtr = mNativePtr;
    mNativePtr = 0;
    mRuntimeDetached = true;
    nativeDestroy(nativePtr);
  }

  @CalledByNative
  private void onHostScriptEntryResult(String status, String message) {
    UIThreadUtils.runOnUiThread(() -> mEntryResultListener.onEntryResult(status, message));
  }

  private static native long nativeCreate(LynxHostScriptRuntimeBinding binding);
  private static native void nativeOnRuntimeAttach(long nativePtr, long napiEnv);
  private static native void nativeOnRuntimeDetach(long nativePtr);
  private static native void nativeDestroy(long nativePtr);
}
