// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.transfer;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

/** Synchronous UI-thread lifecycle callbacks for one managed transfer. */
@UiThread
public abstract class LynxTransferClient {
  /** Called before the create command is dispatched and before loadTransfer returns. */
  public void onLoadStart(@NonNull LynxTransferredView view) {}

  /** Called after native content is first bound to the container. */
  public void onLoadSuccess(@NonNull LynxTransferredView view) {}

  /** Called after the attached container first draws its content, before screen presentation. */
  public void onFirstScreen(@NonNull LynxTransferredView view) {}

  /** Called once after the SDK handle is invalidated and its native content is removed. */
  public void onReleased(@NonNull LynxTransferredView view, @NonNull TransferReleaseReason reason) {
  }
}
