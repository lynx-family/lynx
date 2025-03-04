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

/**
 * CompactArrayBuffer is an optimized sparse array format for transferring array
 * objects between C++ and other VMs. The implementation of this array is optimized to:
 * - be compact to optimize space when sparse (sparse is the common case).
 * - be accessible through JNI with zero/minimal copying via ByteBuffer.
 *
 * See <react/android/mapbuffer/compact_array_buffer.h> for more information and native
 * implementation.
 *
 * Limitations:
 * - Counting are usually sized as 2 bytes, with each buffer supporting up to 65536 entries as a
 * result.
 * - CompactArrayBuffer won't check DataType when get[Data], please ensure that the type matches
 */
public interface CompactArrayBuffer extends Iterable<CompactArrayBuffer.Entry> {
  /**
   * Number of elements inserted into current CompactArrayBuffer.
   *
   * @return number of elements in the [CompactArrayBuffer]
   */
  int count();

  /**
   * Provides parsed [Int] value if the entry for given index
   *
   * @param index index that [Int] value at
   * @return value associated with the requested key
   */
  int getInt(int index);

  long getLong(int index);

  double getDouble(int index);

  /**
   * Provides parsed [String] value if the entry for given index
   *
   * @param index index that [String] value at
   * @return value associated with the requested key
   */
  String getString(int index);

  /**
   * Iterable entry representing parsed CompactArrayBuffer values
   * Entry won't check DataType when get[Data], please ensure that the type matches
   */
  interface Entry {
    /**
     * Entry value represented as [Int]
     */
    int getInt();

    double getDouble();

    long getLong();

    /**
     * Entry value represented as [String]
     */
    String getString();
  }
}
