// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Arrays;

/**
 * Request object for querying node metadata from a {@link LynxView}.
 */
public final class LynxNodeInfoRequest {
  @NonNull private int[] mSigns = new int[0];

  public LynxNodeInfoRequest() {}

  public void setSigns(@Nullable int[] signs) {
    mSigns = signs == null ? new int[0] : Arrays.copyOf(signs, signs.length);
  }

  /**
   * Returns the signs that should be queried.
   */
  @NonNull
  public int[] getSigns() {
    return Arrays.copyOf(mSigns, mSigns.length);
  }
}
