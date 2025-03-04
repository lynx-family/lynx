// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

public class BackgroundColorSpan extends android.text.style.BackgroundColorSpan {
  public BackgroundColorSpan(int color) {
    super(color);
  }

  @Override
  public boolean equals(Object o) {
    if (o instanceof BackgroundColorSpan) {
      BackgroundColorSpan span = (BackgroundColorSpan) o;
      return getBackgroundColor() == span.getBackgroundColor();
    }
    return false;
  }

  @Override
  public int hashCode() {
    return 31 + getBackgroundColor();
  }
}
