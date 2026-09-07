// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import androidx.annotation.RestrictTo;
import com.lynx.devtoolwrapper.LynxBaseInspectorController;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.LynxView;
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

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public LynxNetworkRequestObserver getNetworkRequestObserver() {
    LynxContext context = weakContext.get();
    if (context != null) {
      LynxView view = context.getLynxView();
      LynxBaseInspectorController controller =
          view != null ? view.getBaseInspectorController() : null;
      return controller != null ? controller.getNetworkRequestObserver() : null;
    }

    LynxBackgroundRuntime runtime = weakRuntime.get();
    LynxDevtool devtool = runtime != null ? runtime.getDevtool() : null;
    LynxBaseInspectorController controller =
        devtool != null ? devtool.getBaseInspectorController() : null;
    return controller != null ? controller.getNetworkRequestObserver() : null;
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
