// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;

/**
 * Metadata for a single queried node.
 */
public final class LynxNodeInfoItem {
  private final int mSign;
  @Nullable private final String mEntryUrl;
  @Nullable private final String mDebugMetadataUrl;

  private LynxNodeInfoItem(int sign, @Nullable String entryUrl, @Nullable String debugMetadataUrl) {
    mSign = sign;
    mEntryUrl = entryUrl;
    mDebugMetadataUrl = debugMetadataUrl;
  }

  public int getSign() {
    return mSign;
  }

  @Nullable
  public String getEntryUrl() {
    return mEntryUrl;
  }

  @Nullable
  public String getDebugMetadataUrl() {
    return mDebugMetadataUrl;
  }

  @CalledByNative
  private static LynxNodeInfoItem create(
      int sign, @Nullable String entryUrl, @Nullable String debugMetadataUrl) {
    return new LynxNodeInfoItem(sign, entryUrl, debugMetadataUrl);
  }
}
