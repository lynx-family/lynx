/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

/**
 * Implementation of Dynamic wrapping a ReadableArray.
 */
public class DynamicFromArray implements Dynamic {
  private final ReadableArray mArray;
  private final int mIndex;

  public DynamicFromArray(ReadableArray array, int index) {
    mArray = array;
    mIndex = index;
  }

  @Deprecated
  /*
   * Use new DynamicFromArray(ReadableArray array, int index) instead of this.
   * This method was used by pool cache mechanism.
   * However DynamicFromArray class is so straightforward that using pool cache costs more time.
   * Now we just leave it here for compatibility.
   */
  public static DynamicFromArray create(ReadableArray array, int index) {
    return new DynamicFromArray(array, index);
  }

  @Override
  @Deprecated
  public void recycle() {
    // Do nothing. Let JVM collect this object.
  }

  @Override
  public boolean isNull() {
    return mArray.isNull(mIndex);
  }

  @Override
  public boolean asBoolean() {
    return mArray.getBoolean(mIndex);
  }

  @Override
  public double asDouble() {
    return mArray.getDouble(mIndex);
  }

  @Override
  public int asInt() {
    return mArray.getInt(mIndex);
  }

  @Override
  public long asLong() {
    return mArray.getLong(mIndex);
  }

  @Override
  public String asString() {
    return mArray.getString(mIndex);
  }

  @Override
  public ReadableArray asArray() {
    return mArray.getArray(mIndex);
  }

  @Override
  public ReadableMap asMap() {
    return mArray.getMap(mIndex);
  }

  @Override
  public byte[] asByteArray() {
    return mArray.getByteArray(mIndex);
  }

  @Override
  public ReadableType getType() {
    return mArray.getType(mIndex);
  }
}
