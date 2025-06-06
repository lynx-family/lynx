// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.memory;

import java.util.Map;

public class MemoryRecord {
  private String mCategory;
  private float mSizeKb;
  private Map<String, String> mDetail = null;

  public MemoryRecord(String category, float sizeKb, Map<String, String> detail) {
    mCategory = category;
    mSizeKb = sizeKb;
    mDetail = detail;
  }

  public static MemoryRecord create(String category, float sizeKb) {
    return new MemoryRecord(category, sizeKb, null);
  }

  public String getCategory() {
    return mCategory;
  }

  public float getSizeKb() {
    return mSizeKb;
  }

  public Map<String, String> getDetails() {
    return mDetail;
  }
}
