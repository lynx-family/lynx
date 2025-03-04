// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

public class AbsoluteSizeSpan extends android.text.style.AbsoluteSizeSpan {
  public AbsoluteSizeSpan(int size) {
    super(size);
  }

  public AbsoluteSizeSpan(int size, boolean dip) {
    super(size, dip);
  }

  @Override
  public boolean equals(Object o) {
    if (o instanceof AbsoluteSizeSpan) {
      AbsoluteSizeSpan span = (AbsoluteSizeSpan) o;
      return span.getDip() == getDip() && span.getSize() == getSize();
    }
    return false;
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = getDip() ? 1 : 0;
    result = result * prime + getSize();
    return result;
  }
}
