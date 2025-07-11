// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.ref.WeakReference;

public class LynxFetchModuleEventSender {
  private WeakReference<LynxContext> weakContext;
  private WeakReference<LynxBackgroundRuntime> weakRuntime;

  public LynxFetchModuleEventSender() {
    weakContext = new WeakReference<>(null);
    weakRuntime = new WeakReference<>(null);
  }
  public void setWeakContext(LynxContext context) {
    weakContext = new WeakReference<>(context);
  }

  public void setWeakRuntime(LynxBackgroundRuntime runtime) {
    weakRuntime = new WeakReference<>(runtime);
  }

  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    LynxContext context = weakContext.get();
    if (context != null) {
      context.sendGlobalEvent(name, params);
      return;
    }

    LynxBackgroundRuntime runtime = weakRuntime.get();
    if (runtime != null) {
      runtime.sendGlobalEvent(name, params);
    }
  }
}
