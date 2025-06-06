// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

public class TimingUtil {
  private static final long initialMillis = System.currentTimeMillis();
  private static final long initialNanos = System.nanoTime();
  public static long currentTimeUs() {
    long elapsedNanos = System.nanoTime() - initialNanos;
    return initialMillis * 1000 + elapsedNanos / 1000;
  }
}
