// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.VisibleForTesting;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.utils.UIThreadUtils;

/** Creation notifications enabled by the debug engine. Does not retain Views or start runtimes. */
final class LynxHostScriptViewHooks {
  // A new token separates queued notifications from earlier enabled periods.
  private static volatile Object sEnabledToken;
  private static long sNextId;

  private LynxHostScriptViewHooks() {}

  @CalledByNative
  private static void setEnabled(boolean enabled) {
    UIThreadUtils.assertOnUiThread();
    if (!enabled) {
      sEnabledToken = null;
    } else if (sEnabledToken == null) {
      sEnabledToken = new Object();
    }
  }

  static boolean isEnabled() {
    return sEnabledToken != null;
  }

  static void onCreated(LynxView view) {
    postCreationNotification(() -> {
      String url = view.getTemplateUrl();
      nativeDispatchCreated(++sNextId, url == null ? "" : url);
    });
  }

  @VisibleForTesting
  static void postCreationNotification(Runnable notification) {
    Object token = sEnabledToken;
    if (token == null) {
      return;
    }
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (sEnabledToken != token) {
        return;
      }
      notification.run();
    });
  }

  private static native void nativeDispatchCreated(long viewId, String url);
}
