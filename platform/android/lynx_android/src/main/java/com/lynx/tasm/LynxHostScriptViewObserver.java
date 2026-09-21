// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.utils.UIThreadUtils;

/** DevTool-gated creation adapter. Does not retain Views or start runtimes. */
final class LynxHostScriptViewObserver extends LynxViewClient {
  private static LynxHostScriptViewObserver sInstance;
  private static long sNextId;

  @CalledByNative
  private static void install() {
    UIThreadUtils.assertOnUiThread();
    if (DevToolLifecycle.getInstance().isEnabled() && sInstance == null) {
      sInstance = new LynxHostScriptViewObserver();
      LynxEnv.inst().addLynxViewClient(sInstance);
    }
  }

  @CalledByNative
  private static void uninstall() {
    UIThreadUtils.assertOnUiThread();
    if (sInstance != null) {
      LynxEnv.inst().removeLynxViewClient(sInstance);
      sInstance = null;
    }
  }

  @Override
  void onLynxViewCreated(LynxView view) {
    if (!DevToolLifecycle.getInstance().isEnabled())
      return;
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (sInstance != this || !DevToolLifecycle.getInstance().isEnabled())
        return;
      String url = view.getTemplateUrl();
      nativeNotifyCreated(Long.toString(++sNextId), url == null ? "" : url);
    });
  }

  private static native void nativeNotifyCreated(String viewId, String url);
}
