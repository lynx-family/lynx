// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core.resource;

import com.lynx.tasm.base.LLog;

/**
 * Base class to make sure LynxResourceCallbackBack Only Be invoked Once.
 */
abstract class GuardedResourceCallback {
  public static final String DOUBLE_INVOKE_ERROR_MSG =
      "Illegal callback invocation from native. The loaded callback can only be invoked once! The url is ";

  private volatile boolean mInvoked = false;
  protected final String mUrl;
  GuardedResourceCallback(String url) {
    this.mUrl = url;
  }

  boolean EnsureInvokedOnce() {
    synchronized (this) {
      if (mInvoked) {
        LLog.e(LynxResourceLoader.TAG, DOUBLE_INVOKE_ERROR_MSG + mUrl);
        return false;
      }
      mInvoked = true;
      return true;
    }
  }
}
