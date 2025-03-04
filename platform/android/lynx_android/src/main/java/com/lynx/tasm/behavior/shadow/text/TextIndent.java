// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableArray;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class TextIndent {
  private static final int TYPE_VALUE_NUMBER = 0;
  private final int mType;
  private final float mValue;

  public TextIndent(ReadableArray arr) {
    mValue = (float) arr.getDouble(0);
    mType = arr.getInt(1);
  }
  public float getValue(float width) {
    return mType == TYPE_VALUE_NUMBER ? mValue : mValue * width;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if (obj == null || getClass() != obj.getClass()) {
      return false;
    }

    TextIndent o = (TextIndent) obj;
    return mValue == o.mValue && mType == o.mType;
  }

  @Override
  public int hashCode() {
    int result = mType;
    result = 31 * result + Float.floatToIntBits(mValue);
    return result;
  }
}
