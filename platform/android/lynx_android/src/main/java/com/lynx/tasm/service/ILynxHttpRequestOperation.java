// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.AnyThread;
import androidx.annotation.Keep;

/**
 * An in-flight http request started by {@link ILynxHttpService}.
 */
@Keep
public interface ILynxHttpRequestOperation {
  /**
   * Cancel the request. It is a no-op if the request is already finished.
   */
  @AnyThread void cancel();
}
