// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.style.UnderlineSpan;

public class LynxUnderlineSpan extends UnderlineSpan {
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }

    LynxUnderlineSpan that = (LynxUnderlineSpan) o;

    if (this.getSpanTypeId() != that.getSpanTypeId()) {
      return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = getSpanTypeId() * prime;
    return result;
  }
}
