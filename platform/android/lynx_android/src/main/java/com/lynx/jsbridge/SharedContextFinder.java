// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.Nullable;
import java.lang.ref.WeakReference;
import java.util.concurrent.ConcurrentHashMap;

public class SharedContextFinder implements IContextFinder {
  /**
   * mContextMap is used to store LynxContext by instanceId.
   */
  private final ConcurrentHashMap<String, WeakReference<Context>> mContextMap =
      new ConcurrentHashMap<>();

  @Nullable
  @Override
  public WeakReference<Context> findContext(String instanceId) {
    return mContextMap.get(instanceId);
  }

  @Override
  public void registerContext(String instanceId, WeakReference<Context> context) {
    mContextMap.put(instanceId, context);
  }
}
