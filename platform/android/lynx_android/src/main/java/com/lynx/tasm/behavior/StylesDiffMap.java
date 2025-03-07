// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.MapBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBufferWrapper;

public class StylesDiffMap {
  public final ReadableMap mBackingMap;

  // storing the initial css styles if mapBuffer enabled.
  private MapBuffer mBackingStyles = null;

  public StylesDiffMap(ReadableMap props) {
    mBackingMap = props;
  }

  public StylesDiffMap(ReadableMap props, MapBuffer styles) {
    mBackingMap = props;
    mBackingStyles = styles;
  }

  public boolean hasKey(String name) {
    return mBackingMap.hasKey(name);
  }

  public boolean hasKey(int key) {
    if (mBackingStyles != null) {
      return mBackingStyles.contains(key);
    }
    return false;
  }

  public boolean isEmpty() {
    return mBackingMap == null || mBackingMap.size() == 0;
  }

  public boolean isNull(String name) {
    return mBackingMap.isNull(name);
  }

  public boolean getBoolean(String name, boolean restoreNullToDefaultValue) {
    return mBackingMap.isNull(name) ? restoreNullToDefaultValue : mBackingMap.getBoolean(name);
  }

  public double getDouble(String name, double restoreNullToDefaultValue) {
    return mBackingMap.isNull(name) ? restoreNullToDefaultValue : mBackingMap.getDouble(name);
  }

  public float getFloat(String name, float restoreNullToDefaultValue) {
    return mBackingMap.isNull(name) ? restoreNullToDefaultValue
                                    : (float) mBackingMap.getDouble(name);
  }

  public int getInt(String name, int restoreNullToDefaultValue) {
    return mBackingMap.isNull(name) ? restoreNullToDefaultValue : mBackingMap.getInt(name);
  }

  @Nullable
  public String getString(String name) {
    return mBackingMap.getString(name);
  }

  @Nullable
  public ReadableArray getArray(String key) {
    return mBackingMap.getArray(key);
  }

  @Nullable
  public ReadableArray getArray(int key) {
    if (mBackingStyles != null) {
      return new ReadableMapBufferWrapper(mBackingStyles.getMapBuffer(key));
    }
    return null;
  }

  @Nullable
  public ReadableMap getMap(String key) {
    return mBackingMap.getMap(key);
  }

  @Nullable
  public Dynamic getDynamic(String key) {
    return mBackingMap.getDynamic(key);
  }

  public MapBuffer getStyleMap() {
    return mBackingStyles;
  }

  @Override
  public String toString() {
    return "{ " + getClass().getSimpleName() + ": " + mBackingMap.toString() + " }";
  }
}
