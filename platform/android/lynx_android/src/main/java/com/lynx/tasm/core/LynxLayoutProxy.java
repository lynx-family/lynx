// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import java.lang.Runnable;

public class LynxLayoutProxy {
  private long mNativePtr;

  public LynxLayoutProxy(long lynxShellPtr) {
    initNative(lynxShellPtr);
  }

  protected void initNative(long lynxShellPtr) {
    mNativePtr = nativeCreate(lynxShellPtr);
  }

  public void runOnLayoutThread(Runnable runnable) {
    if (mNativePtr != 0) {
      nativeRunOnLayoutThread(mNativePtr, runnable);
    }
  }

  public void destroy() {
    nativeRelease(mNativePtr);
    mNativePtr = 0;
  }

  private native long nativeCreate(long lynxShellPtr);

  private native void nativeRelease(long nativePtr);

  private native void nativeRunOnLayoutThread(long nativePtr, Runnable runnable);
}
