// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import com.lynx.tasm.base.LLog;

/**
 * Caution: destroy() should be called to release native memory.
 */
final class LynxWhiteBoard {
  public static final String TAG = "LynxWhiteBoard";
  private long mPtr = 0;

  LynxWhiteBoard() {
    if (LynxEnv.inst().isNativeLibraryLoaded()) {
      mPtr = nativeCreate();
    } else {
      LLog.e(TAG, "LynxWhiteBoard create failed, since LynxEnv init failed.");
    }
  }

  long getPtr() {
    return mPtr;
  }

  /**
   * It's necessary to invoke destroy method to avoid mem leak.
   */
  public void destroy() {
    if (mPtr != 0) {
      nativeDestroy(mPtr);
      mPtr = 0;
    }
  }

  private native long nativeCreate();

  private native void nativeDestroy(long ptr);
}
