/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;
import java.util.ArrayList;

/**
 * Interface for an array that allows typed access to its members. Used to pass parameters from JS
 * to Java.
 */
@Keep
public interface ReadableArray {
  int size();
  boolean isNull(int index);
  boolean getBoolean(int index);
  double getDouble(int index);
  byte getByte(int index);
  short getShort(int index);
  long getLong(int index);
  char getChar(int index);
  int getInt(int index);
  String getString(int index);
  ReadableArray getArray(int index);
  ReadableMap getMap(int index);
  byte[] getByteArray(int index);
  PiperData getPiperData(int index);
  Dynamic getDynamic(int index);
  ReadableType getType(int index);
  @Deprecated
  /*
   * Use asArrayList() for better performance when you want to READ and ONLY READ the array.
   */
  ArrayList<Object> toArrayList();
  /*
   * Return this reference directly.
   * Please be careful if you are gonna modify it.
   * Besides you can always make a copy of it if necessary.
   */
  ArrayList<Object> asArrayList();
}
