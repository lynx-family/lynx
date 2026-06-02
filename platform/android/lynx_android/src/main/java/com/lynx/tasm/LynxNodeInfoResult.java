// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Result object returned by {@link LynxView#getNodeInfos(LynxNodeInfoRequest,
 * LynxNodeInfoCallback)}.
 */
public final class LynxNodeInfoResult {
  private final int mErrCode;
  private final List<LynxNodeInfoItem> mItems;
  @Nullable private final String mErrMsg;

  private LynxNodeInfoResult(
      @NonNull List<LynxNodeInfoItem> items, int errCode, @Nullable String errMsg) {
    mItems = items;
    mErrCode = errCode;
    mErrMsg = errMsg;
  }

  @NonNull
  public List<LynxNodeInfoItem> getItems() {
    return mItems;
  }

  public boolean succeed() {
    return mErrCode == LynxUIMethodConstants.SUCCESS;
  }

  public int getErrCode() {
    return mErrCode;
  }

  @Nullable
  public String getErrMsg() {
    return mErrMsg;
  }

  static LynxNodeInfoResult empty() {
    return new LynxNodeInfoResult(Collections.emptyList(), LynxUIMethodConstants.SUCCESS, null);
  }

  static LynxNodeInfoResult error(int errCode, @Nullable String errMsg) {
    return new LynxNodeInfoResult(Collections.emptyList(), errCode, errMsg);
  }

  @CalledByNative
  private static LynxNodeInfoResult create(
      LynxNodeInfoItem[] items, int errCode, @Nullable String errMsg) {
    List<LynxNodeInfoItem> typedItems = Collections.emptyList();
    if (items == null || items.length == 0) {
      return new LynxNodeInfoResult(typedItems, errCode, errMsg);
    }
    typedItems = Collections.unmodifiableList(Arrays.asList(Arrays.copyOf(items, items.length)));
    return new LynxNodeInfoResult(typedItems, errCode, errMsg);
  }
}
