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

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.CalledByNative;
import java.nio.ByteBuffer;
import java.util.Iterator;

/**
 * Read-only implementation of the [CompactArrayBuffer], imported from C++ environment. Use
 * `<react/android/mapbuffer/readable_compact_array_buffer.h> to create it.
 *
 * See [CompactArrayBuffer] documentation for more details
 */
@AnyThread
public class ReadableCompactArrayBuffer extends ReadableBaseBuffer implements CompactArrayBuffer {
  // 4 bytes = 4 (value)
  private static final int BUCKET_SIZE = 8;

  private static final int TYPE_OFFSET = 0;

  // 4 bytes = 2 (key) + 2 (type)
  private static final int VALUE_OFFSET = 0;

  private ReadableCompactArrayBuffer(ByteBuffer buffer, int count) {
    super(buffer, count, TYPE_OFFSET, VALUE_OFFSET);
  }

  @CalledByNative
  static public ReadableCompactArrayBuffer fromByteBufferWithCount(byte[] bytes, int count) {
    if (bytes == null) {
      return null;
    }
    return new ReadableCompactArrayBuffer(ByteBuffer.wrap(bytes), count);
  }

  @Override
  protected int getKeyOffsetForBucketIndex(int bucketIndex) {
    return HEADER_SIZE + BUCKET_SIZE * bucketIndex;
  }

  @Override
  public int getInt(int index) {
    return readIntValue(getKeyOffsetForBucketIndex(index));
  }

  @Override
  public long getLong(int index) {
    return readLongValue(getKeyOffsetForBucketIndex(index));
  }

  @Override
  public double getDouble(int index) {
    return readDoubleValue(getKeyOffsetForBucketIndex(index));
  }

  @Override
  public String getString(int index) {
    return readStringValue(getKeyOffsetForBucketIndex(index));
  }

  @NonNull
  @Override
  public Iterator<CompactArrayBuffer.Entry> iterator() {
    return new Iterator<CompactArrayBuffer.Entry>() {
      Iterator<BaseBufferEntry> iterator = baseIterator();

      @Override
      public boolean hasNext() {
        return iterator.hasNext();
      }

      @Override
      public CompactArrayBuffer.Entry next() {
        return iterator.next();
      }
    };
  }
}
