/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;

/**
 * Interface for a mutable map. Used to pass arguments from Java to JS.
 */
@Keep
public interface WritableMap extends ReadableMap {
  void putNull(String key);
  void putBoolean(String key, boolean value);
  void putDouble(String key, double value);
  void putInt(String key, int value);
  void putLong(String key, long value);
  void putString(String key, String value);
  void putArray(String key, WritableArray value);
  void putMap(String key, WritableMap value);
  void putByteArrayAsString(byte[] key, byte[] value);
  void putByteArray(String key, byte[] value);
  void putPiperData(String key, PiperData value);
  void merge(ReadableMap source);
}
