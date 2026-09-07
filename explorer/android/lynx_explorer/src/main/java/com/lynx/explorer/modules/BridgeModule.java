// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.core.LynxThreadPool;

/** Minimal bridge implementation used by native timing test cards. */
public class BridgeModule extends LynxModule {
  public static final String NAME = "bridge";

  public BridgeModule(Context context) {
    super(context);
  }

  @LynxMethod
  public void call(String name, ReadableMap map, Callback callback) {
    if (callback == null) {
      return;
    }

    switch (name) {
      case "timing":
        callback.invoke(map);
        break;
      case "asyncTiming":
        LynxThreadPool.getBriefIOExecutor().execute(() -> callback.invoke(map));
        break;
      default:
        break;
    }
  }
}
