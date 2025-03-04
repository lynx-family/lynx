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
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.PiperData;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import java.util.ArrayList;

/**
 * An adapter for mapBuffer to ReadableMap.
 */
public class ReadableMapBufferWrapper implements ReadableArray {
  private final MapBuffer mBackingMap;

  public ReadableMapBufferWrapper(MapBuffer mapBuffer) {
    mBackingMap = mapBuffer;
  }

  @Override
  public boolean isNull(int key) {
    return false;
  }

  @Override
  public boolean getBoolean(int key) {
    return mBackingMap.getBoolean(key);
  }

  @Override
  public double getDouble(int key) {
    return mBackingMap.getDouble(key);
  }

  @Override
  public byte getByte(int index) {
    return (byte) getInt(index);
  }

  @Override
  public short getShort(int index) {
    return (short) getInt(index);
  }

  @Override
  public int getInt(int key) {
    return mBackingMap.getInt(key);
  }

  @Override
  public long getLong(int key) {
    return mBackingMap.getLong(key);
  }

  @Override
  public char getChar(int index) {
    return 0;
  }

  @Override
  public String getString(int key) {
    return mBackingMap.getString(key);
  }

  @Override
  public ReadableArray getArray(int key) {
    return new ReadableMapBufferWrapper(mBackingMap.getMapBuffer(key));
  }

  @Override
  public ReadableMap getMap(int key) {
    return null;
  }

  @Override
  public byte[] getByteArray(int key) {
    return new byte[0];
  }

  @Override
  public PiperData getPiperData(int key) {
    // Attention: PiperData is no longer supported in map buffer.
    return null;
  }

  @Override
  public Dynamic getDynamic(int key) {
    return new DynamicFromMapBuffer(this, key);
  }

  private static ReadableType MapBufferDataTypeToReadableType(MapBuffer.DataType type) {
    switch (type) {
      case STRING:
        return ReadableType.String;
      case DOUBLE:
        return ReadableType.Number;
      case ARRAY:
        return ReadableType.Array;
      case NULL:
        return ReadableType.Null;
      case LONG:
        return ReadableType.Long;
      case INT:
        return ReadableType.Int;
      case BOOL:
        return ReadableType.Boolean;
    }
    return ReadableType.Null;
  }

  @Override
  public ReadableType getType(int key) {
    return MapBufferDataTypeToReadableType(mBackingMap.getType(key));
  }

  @Override
  public ArrayList<Object> toArrayList() {
    // TODO(nihao.royal) make a lazy convert to ArrayList.
    return null;
  }

  @Override
  public ArrayList<Object> asArrayList() {
    // TODO(nihao.royal) make a lazy convert to ArrayList.
    return null;
  }

  @Override
  public int size() {
    return mBackingMap.count();
  }
}
