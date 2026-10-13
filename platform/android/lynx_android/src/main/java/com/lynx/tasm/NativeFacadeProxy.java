// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.concurrent.atomic.AtomicReference;

/** A stable entry point to the current engine's replaceable Java facade. */
final class NativeFacadeProxy {
  private final AtomicReference<NativeFacade> mNativeFacade = new AtomicReference<>();

  void attach(@NonNull NativeFacade facade) {
    mNativeFacade.set(facade);
  }

  /** Transfers the current facade to the caller for per-engine destruction. */
  @Nullable
  synchronized NativeFacade detach() {
    return mNativeFacade.getAndSet(null);
  }

  /**
   * Captures one engine's facade for JNI binding or a template load. Keep this snapshot with
   * that engine's native pointer and lifecycle; never bind JNI callbacks to the proxy itself.
   * Native operations still require their existing native lifecycle checks.
   */
  @Nullable
  NativeFacade getNativeFacade() {
    return mNativeFacade.get();
  }

  boolean shouldSendEventToMainThread() {
    NativeFacade facade = mNativeFacade.get();
    return facade == null || facade.shouldSendEventToMainThread();
  }

  synchronized boolean setTemplateLoadClient(LynxViewClient client) {
    NativeFacade facade = mNativeFacade.get();
    if (facade == null) {
      return false;
    }
    facade.setTemplateLoadClient(client);
    return true;
  }

  void reportError(LynxError error) {
    NativeFacade facade = mNativeFacade.get();
    if (facade != null) {
      // An already captured callback may finish during detach. Do not hold a lock across
      // client code, which may reenter the renderer or wait for another thread.
      facade.reportError(error);
    }
  }
}
