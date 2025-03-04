/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge.mapbuffer;

import java.util.List;

/**
 * MapBuffer is an optimized sparse array format for transferring props-like data between C++ and
 * JNI. It is designed to:
 * - be compact to optimize space when sparse (sparse is the common case).
 * - be accessible through JNI with zero/minimal copying.
 * - work recursively for nested maps/arrays.
 * - support dynamic types that map to JSON.
 * - have minimal APK size and build time impact.
 *
 * See <react/android/mapbuffer/map_buffer.h> for more information and native implementation.
 *
 * Limitations:
 * - Keys are usually sized as 2 bytes, with each buffer supporting up to 65536 entries as a result.
 * - O(log(N)) random key access for native buffers due to selected structure. Faster access can be
 *   achieved by retrieving [MapBuffer.Entry] with [entryAt] on known offsets.
 */
public interface MapBuffer extends Iterable<MapBuffer.Entry> {
  /**
   * Data types supported by [MapBuffer]. Keep in sync with definition in
   * `<react/android/mapbuffer/map_buffer.h>`, as enum serialization relies on correct order.
   */
  enum DataType { NULL, BOOL, INT, LONG, DOUBLE, STRING, ARRAY }

  /**
   * Number of elements inserted into current MapBuffer.
   *
   * @return number of elements in the [MapBuffer]
   */
  int count();

  /**
   * Checks whether entry for given key exists in MapBuffer.
   *
   * @param key key to lookup the entry
   * @return whether entry for the given key exists in the MapBuffer.
   */
  boolean contains(int key);

  /**
   * Provides offset of the key to use for [entryAt], for cases when offset is not statically known
   * but can be cached.
   *
   * @param key key to lookup offset for
   * @return offset for the given key to be used for entry access, -1 if key wasn't found.
   */
  int getKeyOffset(int key);
  /**
   * Provides parsed access to a MapBuffer without additional lookups for provided offset.
   *
   * @param offset offset of entry in the MapBuffer structure. Can be looked up for known keys with
   *   [getKeyOffset].
   * @return parsed entry for structured access for given offset
   */
  MapBuffer.Entry entryAt(int offset);

  /**
   * Provides parsed [DataType] annotation associated with the given key.
   *
   * @param key key to lookup type for
   * @return data type of the given key.
   * @throws IllegalArgumentException if the key doesn't exist
   */
  DataType getType(int key);

  /**
   * Provides parsed [Boolean] value if the entry for given key exists with [DataType.BOOL] type
   *
   * @param key key to lookup [Boolean] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  boolean getBoolean(int key);

  /**
   * Provides parsed [Int] value if the entry for given key exists with [DataType.INT] type
   *
   * @param key key to lookup [Int] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  int getInt(int key);

  /**
   * Provides parsed [long] value if the entry for given key exists with [DataType.INT] type
   *
   * @param key key to lookup [Int] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  long getLong(int key);

  /**
   * Provides parsed [Double] value if the entry for given key exists with [DataType.DOUBLE] type
   *
   * @param key key to lookup [Double] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  double getDouble(int key);

  /**
   * Provides parsed [String] value if the entry for given key exists with [DataType.STRING] type
   *
   * @param key key to lookup [String] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  String getString(int key);

  /**
   * Provides parsed [MapBuffer] value if the entry for given key exists with [DataType.MAP] type
   *
   * @param key key to lookup [MapBuffer] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  MapBuffer getMapBuffer(int key);

  /**
   * Provides parsed [List<MapBuffer>] value if the entry for given key exists with [DataType.MAP]
   * type
   *
   * @param key key to lookup [List<MapBuffer>] value for
   * @return value associated with the requested key
   * @throws IllegalArgumentException if the key doesn't exist
   * @throws IllegalStateException if the data type doesn't match
   */
  List<MapBuffer> getMapBufferList(int key);

  /**
   * Iterable entry representing parsed MapBuffer values
   * Entry won't check DataType when get[Data], please ensure that the type matches
   */
  interface Entry extends CompactArrayBuffer.Entry {
    /**
     * Key of the given entry. Usually represented as 2 byte unsigned integer with the value range
     * of [0,65536)
     */
    int getKey();

    /** Parsed [DataType] of the entry */
    DataType getType();

    /**
     * Entry value represented as [Boolean]
     */
    boolean getBoolean();

    /**
     * Entry value represented as [Long]
     */
    long getLong();

    /**
     * Entry value represented as [Double]
     */
    double getDouble();

    /**
     * Entry value represented as [MapBuffer]
     */
    MapBuffer getMapBuffer();
  }
}
