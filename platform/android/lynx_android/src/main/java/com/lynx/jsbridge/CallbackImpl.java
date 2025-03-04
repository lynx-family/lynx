// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import androidx.annotation.Keep;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.WritableArray;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;

/**
 * Implementation of javascript callback function that use Bridge to schedule method execution
 */
@Keep
public final class CallbackImpl implements Callback {
  private long mNativePtr;
  private boolean mInvoked;

  @Keep
  public CallbackImpl(long ptr) {
    assert (ptr != 0);
    mNativePtr = ptr;
    mInvoked = false;
  }

  @Keep
  @Override
  public void invoke(Object... args) {
    if (mInvoked) {
      LLog.report("LynxModule",
          "Illegal callback invocation from native "
              + "module. This callback type only permits a single invocation from "
              + "native code.");
      return;
    }
    if (mNativePtr == 0) {
      LLog.e("LynxModule", "callback invoke failed: mNativePtr is NULL");
      return;
    }
    nativeInvoke(mNativePtr, JavaOnlyArray.of(args));
    mInvoked = true;
  }

  @Keep
  public void invokeCallback(Object... args) {
    if (mInvoked) {
      LLog.report("LynxModule",
          "Illegal callback invocation from native "
              + "module. This callback type only permits a single invocation from "
              + "native code.");
      return;
    }
    if (mNativePtr == 0) {
      LLog.e("LynxModule", "callback invoke failed: mNativePtr is NULL");
      return;
    }
    nativeInvoke(mNativePtr, JavaOnlyArray.of(args));
    mInvoked = true;
  }

  @Keep private native void nativeInvoke(long nativePtr, WritableArray array);
  @Keep private native void nativeReleaseNativePtr(long nativePtr);

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    nativeReleaseNativePtr(mNativePtr);
  }

  @Keep
  @CalledByNative
  public void resetNativePtr() {
    mNativePtr = 0;
  }
}
