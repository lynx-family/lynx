// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import com.lynx.tasm.base.CalledByNative;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Result object returned by {@link LynxView#getNodeInfos(LynxNodeInfoRequest,
 * LynxNodeInfoCallback)}.
 */
public final class LynxNodeInfoResult {
  private final List<LynxNodeInfoItem> mItems;

  private LynxNodeInfoResult(@NonNull List<LynxNodeInfoItem> items) {
    mItems = items;
  }

  @NonNull
  public List<LynxNodeInfoItem> getItems() {
    return mItems;
  }

  static LynxNodeInfoResult empty() {
    return new LynxNodeInfoResult(Collections.emptyList());
  }

  @CalledByNative
  private static LynxNodeInfoResult create(LynxNodeInfoItem[] items) {
    if (items == null || items.length == 0) {
      return empty();
    }
    return new LynxNodeInfoResult(
        Collections.unmodifiableList(Arrays.asList(Arrays.copyOf(items, items.length))));
  }
}
