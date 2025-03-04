// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ThreadUtils {
  private static final ExecutorService sThreadPool = Executors.newCachedThreadPool();

  public static ExecutorService getThreadPool() {
    return sThreadPool;
  }
}
