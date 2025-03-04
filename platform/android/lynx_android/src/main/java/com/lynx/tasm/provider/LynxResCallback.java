// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;

/**
 * Use @{@link LynxResourceCallback} instead
 */
@Deprecated
public interface LynxResCallback {
  @AnyThread void onSuccess(@NonNull LynxResResponse response);

  @AnyThread void onFailed(@NonNull LynxResResponse response);
}
