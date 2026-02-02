// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.base;

import com.lynx.base.memory.MemoryPressureCallback;
import com.lynx.base.memory.MemoryPressureLevel;
import java.util.concurrent.CopyOnWriteArrayList;

public final class MemoryPressureCallbackDispatcher {
  private static final MemoryPressureCallbackDispatcher INSTANCE =
      new MemoryPressureCallbackDispatcher();

  private final CopyOnWriteArrayList<MemoryPressureCallback> callbacks =
      new CopyOnWriteArrayList<>();

  private MemoryPressureCallbackDispatcher() {}

  public static MemoryPressureCallbackDispatcher getInstance() {
    return INSTANCE;
  }

  public void addCallback(MemoryPressureCallback callback) {
    callbacks.addIfAbsent(callback);
  }

  public void removeCallback(MemoryPressureCallback callback) {
    callbacks.remove(callback);
  }

  public void notifyMemoryPressure(@MemoryPressureLevel int pressure) {
    for (MemoryPressureCallback callback : callbacks) {
      callback.onPressure(pressure);
    }
  }
}
