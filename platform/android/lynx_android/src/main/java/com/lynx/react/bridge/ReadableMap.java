/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;
import java.util.HashMap;

/**
 * Interface for a map that allows typed access to its members. Used to pass parameters from JS to
 * Java.
 */
@Keep
public interface ReadableMap {
  boolean hasKey(String name);
  boolean isNull(String name);

  // Not safe as crash will happen while type does not match or key does not exist
  boolean getBoolean(String name);
  double getDouble(String name);
  int getInt(String name);
  long getLong(String name);
  String getString(String name);
  ReadableArray getArray(String name);
  ReadableMap getMap(String name);
  byte[] getByteArray(String name);
  PiperData getPiperData(String name);

  // Get method with default value, this is a safe method
  boolean getBoolean(String name, boolean defaultValue);
  double getDouble(String name, double defaultValue);
  int getInt(String name, int defaultValue);
  long getLong(String name, long defaultValue);
  String getString(String name, String defaultValue);
  ReadableArray getArray(String name, ReadableArray defaultValue);
  ReadableMap getMap(String name, ReadableMap defaultValue);
  byte[] getByteArray(String name, byte[] defaultValue);
  PiperData getPiperData(String name, PiperData defaultValue);

  Dynamic getDynamic(String name);
  ReadableType getType(String name);
  ReadableMapKeySetIterator keySetIterator();

  @Deprecated
  /*
   * Use asHashMap() for better performance when you want to READ and ONLY READ the map.
   */
  HashMap<String, Object> toHashMap();
  /*
   * Return this reference directly.
   * Please be careful if you are gonna modify it.
   * Besides you can always make a copy of it if necessary.
   */
  HashMap<String, Object> asHashMap();

  int size();
}
