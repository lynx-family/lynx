/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge.mapbuffer;

import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;

public class DynamicFromMapBuffer implements Dynamic {
  private final ReadableMapBufferWrapper mMap;
  private final int mKey;

  public DynamicFromMapBuffer(ReadableMapBufferWrapper map, int key) {
    this.mMap = map;
    this.mKey = key;
  }

  @Override
  public boolean isNull() {
    return mMap.isNull(mKey);
  }

  @Override
  public boolean asBoolean() {
    return mMap.getBoolean(mKey);
  }

  @Override
  public double asDouble() {
    return mMap.getDouble(mKey);
  }

  @Override
  public int asInt() {
    return mMap.getInt(mKey);
  }

  @Override
  public long asLong() {
    return mMap.getLong(mKey);
  }

  @Override
  public String asString() {
    return mMap.getString(mKey);
  }

  @Override
  public ReadableArray asArray() {
    return mMap.getArray(mKey);
  }

  @Override
  public ReadableMap asMap() {
    return mMap.getMap(mKey);
  }

  @Override
  public byte[] asByteArray() {
    return mMap.getByteArray(mKey);
  }

  @Override
  public ReadableType getType() {
    return mMap.getType(mKey);
  }

  @Override
  public void recycle() {}
}
