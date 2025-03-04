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

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.CalledByNative;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;

/**
 * Read-only implementation of the [MapBuffer], imported from C++ environment. Use
 * `<react/android/mapbuffer/readable_map_buffer.h> to create it.
 *
 * See [MapBuffer] documentation for more details
 */
@AnyThread
public class ReadableMapBuffer extends ReadableBaseBuffer implements MapBuffer {
  public static final int INDEX_UN_EXISTED = -1;

  // 12 bytes = 2 (key) + 2 (type) + 8 (value)
  private static final int BUCKET_SIZE = 12;

  // 2 bytes = 2 (key)
  private static final int TYPE_OFFSET = 2;

  // 4 bytes = 2 (key) + 2 (type)
  private static final int VALUE_OFFSET = 4;

  ReadableMapBuffer(ByteBuffer buffer, int count) {
    super(buffer, count, TYPE_OFFSET, VALUE_OFFSET);
  }

  @CalledByNative
  static ReadableMapBuffer fromByteBufferWithCount(byte[] bytes, int count) {
    if (bytes == null) {
      return null;
    }
    return new ReadableMapBuffer(ByteBuffer.wrap(bytes), count);
  }

  private int getBucketIndexForKey(int intKey) {
    int lo = 0;
    int hi = count() - 1;
    while (lo <= hi) {
      int mid = (lo + hi) >> 1;
      int midVal = readUnsignedShort(getKeyOffsetForBucketIndex(mid));
      if (midVal < intKey) {
        lo = mid + 1;
      } else if (midVal > intKey) {
        hi = mid - 1;
      } else {
        return mid;
      }
    }
    return INDEX_UN_EXISTED;
  }

  private MapBuffer.DataType readDataType(int bucketIndex) {
    int value = readUnsignedShort(getKeyOffsetForBucketIndex(bucketIndex) + TYPE_OFFSET);
    return MapBuffer.DataType.values()[value];
  }

  private int getTypedValueOffsetForKey(int key, MapBuffer.DataType expected) {
    int bucketIndex = getBucketIndexForKey(key);
    if (bucketIndex == INDEX_UN_EXISTED) {
      return INDEX_UN_EXISTED;
    }

    DataType dataType = readDataType(bucketIndex);
    if (dataType != expected) {
      throw new RuntimeException(
          "Expected " + expected + " for key: " + key + ", found " + dataType + " instead.");
    }
    return getKeyOffsetForBucketIndex(bucketIndex) + VALUE_OFFSET;
  }

  private int getTypedValueOffsetForBucketIndex(int bucketIndex, MapBuffer.DataType expected) {
    DataType dataType = readDataType(bucketIndex);
    if (dataType != expected) {
      throw new RuntimeException("Expected " + expected + ", found " + dataType + " instead.");
    }
    return getKeyOffsetForBucketIndex(bucketIndex) + VALUE_OFFSET;
  }

  private List<ReadableMapBuffer> readMapBufferListValue(int position) {
    return null;
  }

  @Override
  protected int getKeyOffsetForBucketIndex(int bucketIndex) {
    return HEADER_SIZE + BUCKET_SIZE * bucketIndex;
  }

  @Override
  public boolean contains(int key) {
    return getBucketIndexForKey(key) != INDEX_UN_EXISTED;
  }

  @Override
  public int getKeyOffset(int key) {
    return getBucketIndexForKey(key);
  }

  @Override
  public MapBuffer.Entry entryAt(int offset) {
    return new BaseBufferEntry(getKeyOffsetForBucketIndex(offset));
  }

  @Override
  public DataType getType(int key) {
    int bucketIndex = getBucketIndexForKey(key);
    return readDataType(bucketIndex);
  }

  @Override
  public int getInt(int key) {
    return readIntValue(getTypedValueOffsetForKey(key, DataType.INT));
  }

  /**
   * getInt with defaultValue if not exist
   * @param key
   * @param defaultValue
   * @return
   */
  public int getInt(int key, int defaultValue) {
    int bucketIndexForKey = getBucketIndexForKey(key);
    if (bucketIndexForKey == INDEX_UN_EXISTED) {
      return defaultValue;
    }

    return readIntValue(getTypedValueOffsetForBucketIndex(bucketIndexForKey, DataType.INT));
  }

  @Override
  public long getLong(int key) {
    return readLongValue(getTypedValueOffsetForKey(key, DataType.LONG));
  }

  /**
   * getLong with defaultValue if not exist
   * @param key
   * @param defaultValue
   * @return
   */
  public long getLong(int key, long defaultValue) {
    int bucketIndexForKey = getBucketIndexForKey(key);
    if (bucketIndexForKey == INDEX_UN_EXISTED) {
      return defaultValue;
    }

    return readLongValue(getTypedValueOffsetForBucketIndex(bucketIndexForKey, DataType.LONG));
  }

  @Override
  public double getDouble(int key) {
    return readDoubleValue(getTypedValueOffsetForKey(key, DataType.DOUBLE));
  }

  /**
   * getDouble with defaultValue if not exist
   * @param key
   * @param defaultValue
   * @return
   */
  public double getDouble(int key, double defaultValue) {
    int bucketIndexForKey = getBucketIndexForKey(key);
    if (bucketIndexForKey == INDEX_UN_EXISTED) {
      return defaultValue;
    }

    return readDoubleValue(getTypedValueOffsetForBucketIndex(bucketIndexForKey, DataType.DOUBLE));
  }

  @Override
  public String getString(int key) {
    return readStringValue(getTypedValueOffsetForKey(key, DataType.STRING));
  }

  /**
   * getString with defaultValue if not exist
   * @param key
   * @param defaultValue
   * @return
   */
  public String getString(int key, String defaultValue) {
    int bucketIndexForKey = getBucketIndexForKey(key);
    if (bucketIndexForKey == INDEX_UN_EXISTED) {
      return defaultValue;
    }

    return readStringValue(getTypedValueOffsetForBucketIndex(bucketIndexForKey, DataType.STRING));
  }

  @Override
  public boolean getBoolean(int key) {
    return readBooleanValue(getTypedValueOffsetForKey(key, DataType.BOOL));
  }

  /**
   * getBoolean with defaultValue if not exist
   * @param key
   * @param defaultValue
   * @return
   */
  public boolean getBoolean(int key, boolean defaultValue) {
    int bucketIndexForKey = getBucketIndexForKey(key);
    if (bucketIndexForKey == INDEX_UN_EXISTED) {
      return defaultValue;
    }

    return readBooleanValue(getTypedValueOffsetForBucketIndex(bucketIndexForKey, DataType.BOOL));
  }

  @Override
  public ReadableMapBuffer getMapBuffer(int key) {
    return readMapBufferValue(getTypedValueOffsetForKey(key, DataType.ARRAY));
  }

  @Override
  public List<MapBuffer> getMapBufferList(int key) {
    return null;
  }

  @NonNull
  @Override
  public Iterator<MapBuffer.Entry> iterator() {
    return new Iterator<MapBuffer.Entry>() {
      Iterator<BaseBufferEntry> iterator = baseIterator();

      @Override
      public boolean hasNext() {
        return iterator.hasNext();
      }

      @Override
      public MapBuffer.Entry next() {
        return iterator.next();
      }
    };
  }
}
