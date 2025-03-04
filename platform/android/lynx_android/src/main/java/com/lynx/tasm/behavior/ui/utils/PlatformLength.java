// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.utils.FloatUtils;

/**
 * PlatformLength has unit and value.
 * The value can be an array with a sub-length or a float value.
 */
public class PlatformLength {
  private final int mType;
  private float mValue;
  // [value, unit, [value, unit, ...], unit, ...]
  private ReadableArray mArray;

  public PlatformLength(Dynamic value, int type) {
    if (type == StyleConstants.PLATFORM_LENGTH_UNIT_CALC) {
      mArray = value.asArray();
    } else if (type == StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER
        || type == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT) {
      mValue = (float) value.asDouble();
    }
    mType = type;
  }

  public PlatformLength(float value, int type) {
    mValue = value;
    mType = type;
  }

  public float asNumber() {
    return mType == StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER ? mValue : 0;
  }

  public float getValue(float parentValue) {
    return getValueInternal(mArray, mValue, mType, parentValue);
  }

  @Override
  public boolean equals(@Nullable Object obj) {
    if (obj == null) {
      return false;
    }
    PlatformLength other = (PlatformLength) obj;
    if (mType != other.mType) {
      return false;
    }
    if (mType != StyleConstants.PLATFORM_LENGTH_UNIT_CALC) {
      return FloatUtils.floatsEqual(mValue, other.mValue);
    }
    // FIXME: deep equals
    return mArray.asArrayList().equals(other.mArray.asArrayList());
  }

  public boolean isZero() {
    return mType != StyleConstants.PLATFORM_LENGTH_UNIT_CALC && mValue == 0;
  }

  private static float getValueInternal(
      ReadableArray arr, float value, int type, float parentValue) {
    if (type == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT) {
      return value * parentValue;
    } else if (type == StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER) {
      return value;
    } else if (type == StyleConstants.PLATFORM_LENGTH_UNIT_CALC) {
      float ret = 0;
      for (int i = 0; i < arr.size(); i += 2) {
        ReadableArray item_array = null;
        float item_value = 0;
        int item_type = arr.getInt(i + 1);
        if (item_type == StyleConstants.PLATFORM_LENGTH_UNIT_CALC) {
          item_array = arr.getArray(i);
        } else if (item_type == StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER
            || item_type == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT) {
          item_value = (float) arr.getDouble(i);
        }
        ret += getValueInternal(item_array, item_value, item_type, parentValue);
      }
      return ret;
    } else {
      return 0;
    }
  }
}
