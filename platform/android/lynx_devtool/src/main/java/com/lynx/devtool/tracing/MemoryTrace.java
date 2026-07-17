// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.tracing;

import com.lynx.base.memory.MemoryPressureLevel;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;

public class MemoryTrace {
  private long mNativeMemoryTrace = 0;

  private MemoryTrace() {
    mNativeMemoryTrace = nativeCreateMemoryTrace();
  }

  private static class MemoryTraceLoader {
    private static final MemoryTrace INSTANCE = new MemoryTrace();
  }

  public static MemoryTrace getInstance() {
    return MemoryTraceLoader.INSTANCE;
  }

  @CalledByNative
  public void trimMemory() {
    LynxEnv.inst().trimMemory(MemoryPressureLevel.CRITICAL);
    System.gc();
  }

  @CalledByNative
  public void startMemoryTrace() {}

  @CalledByNative
  public void stopMemoryTrace() {}

  public long getNativeMemoryTrace() {
    return mNativeMemoryTrace;
  }

  private native long nativeCreateMemoryTrace();
}
