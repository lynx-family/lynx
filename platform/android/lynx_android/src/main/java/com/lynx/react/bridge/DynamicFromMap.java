/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

/**
 * Implementation of Dynamic wrapping a ReadableMap.
 */
public class DynamicFromMap implements Dynamic {
  private final ReadableMap mMap;
  private final String mName;

  public DynamicFromMap(ReadableMap map, String name) {
    mMap = map;
    mName = name;
  }

  @Deprecated
  /*
   * Use new DynamicFromMap(ReadableMap map, String name) instead of this.
   * This method was used by pool cache mechanism.
   * However DynamicFromMap class is so straightforward that using pool cache costs more time.
   * Now we just leave it here for compatibility.
   */
  public static DynamicFromMap create(ReadableMap map, String name) {
    return new DynamicFromMap(map, name);
  }

  @Override
  @Deprecated
  public void recycle() {
    // Do nothing. Let JVM collect this object.
  }

  @Override
  public boolean isNull() {
    return mMap.isNull(mName);
  }

  @Override
  public boolean asBoolean() {
    return mMap.getBoolean(mName);
  }

  @Override
  public double asDouble() {
    return mMap.getDouble(mName);
  }

  @Override
  public int asInt() {
    return mMap.getInt(mName);
  }

  @Override
  public long asLong() {
    return mMap.getLong(mName);
  }

  @Override
  public String asString() {
    return mMap.getString(mName);
  }

  @Override
  public ReadableArray asArray() {
    return mMap.getArray(mName);
  }

  @Override
  public ReadableMap asMap() {
    return mMap.getMap(mName);
  }

  @Override
  public byte[] asByteArray() {
    return mMap.getByteArray(mName);
  }

  @Override
  public ReadableType getType() {
    return mMap.getType(mName);
  }
}
