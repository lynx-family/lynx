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
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.Iterator;
import java.util.Objects;

/**
 * base class for ReadableMapBuffer and ReadableCompactArrayBuffer
 */
@AnyThread
public abstract class ReadableBaseBuffer {
  protected ByteBuffer mBuffer;
  private int mCount = UNSET_COUNT;
  private int mTypeOffset = 0;
  private int mValueOffset = 0;
  private int mOffsetForDynamicData;

  private static final int UNSET_COUNT = -1;

  // 8 bytes = 2 (alignment) + 2 (count) + 4 (size)
  protected static final int HEADER_SIZE = 8;

  protected ReadableBaseBuffer(ByteBuffer buffer, int count, int typeOffset, int valueOffset) {
    this.mBuffer = buffer;
    this.mCount = count;
    this.mTypeOffset = typeOffset;
    this.mValueOffset = valueOffset;
    readHeader();
  }

  private void readHeader() {
    if (mBuffer != null) {
      mBuffer.order(ByteOrder.nativeOrder());
      if (mCount == UNSET_COUNT) {
        // count
        mBuffer.position(2);
        mCount = readUnsignedShort(mBuffer.position());
      }
      mOffsetForDynamicData = getKeyOffsetForBucketIndex(mCount);
    }
  }

  public int count() {
    return mCount;
  }

  protected short readUnsignedShort(int bufferPosition) {
    return mBuffer.getShort(bufferPosition);
  }

  protected int readIntValue(int bufferPosition) {
    return mBuffer.getInt(bufferPosition);
  }

  protected byte[] readBufferValue(int bufferPosition) {
    int offset = mOffsetForDynamicData + mBuffer.getInt(bufferPosition);
    int sizeOfBuffer = mBuffer.getInt(offset);
    byte[] result = new byte[sizeOfBuffer];
    int bufferOffset = offset + Integer.BYTES;
    mBuffer.position(bufferOffset);
    mBuffer.get(result, 0, sizeOfBuffer);
    return result;
  }

  protected String readStringValue(int bufferPosition) {
    return new String(readBufferValue(bufferPosition), Charset.defaultCharset());
  }

  protected double readDoubleValue(int bufferPosition) {
    return mBuffer.getDouble(bufferPosition);
  }

  protected long readLongValue(int bufferPosition) {
    return mBuffer.getLong(bufferPosition);
  }

  protected boolean readBooleanValue(int bufferPosition) {
    return readIntValue(bufferPosition) == 1;
  }

  protected ReadableMapBuffer readMapBufferValue(int position) {
    return new ReadableMapBuffer(ByteBuffer.wrap(readBufferValue(position)), UNSET_COUNT);
  }

  protected abstract int getKeyOffsetForBucketIndex(int bucketIndex);

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    ReadableBaseBuffer entries = (ReadableBaseBuffer) o;
    return count() == entries.count() && Objects.equals(mBuffer, entries.mBuffer);
  }

  @Override
  public int hashCode() {
    mBuffer.rewind();
    return mBuffer.hashCode();
  }

  protected class BaseBufferEntry implements MapBuffer.Entry {
    protected int bucketOffset = 0;

    BaseBufferEntry(int bucketOffset) {
      this.bucketOffset = bucketOffset;
    }

    @Override
    public int getKey() {
      return readUnsignedShort(bucketOffset);
    }

    @Override
    public MapBuffer.DataType getType() {
      return MapBuffer.DataType.values()[readUnsignedShort(bucketOffset + mTypeOffset)];
    }

    @Override
    public boolean getBoolean() {
      return readBooleanValue(bucketOffset + mValueOffset);
    }

    @Override
    public int getInt() {
      return readIntValue(bucketOffset + mValueOffset);
    }

    @Override
    public long getLong() {
      return readLongValue(bucketOffset + mValueOffset);
    }

    @Override
    public double getDouble() {
      return readDoubleValue(bucketOffset + mValueOffset);
    }

    @Override
    public String getString() {
      return readStringValue(bucketOffset + mValueOffset);
    }

    @Override
    public MapBuffer getMapBuffer() {
      return readMapBufferValue(bucketOffset + mValueOffset);
    }
  }

  protected Iterator<BaseBufferEntry> baseIterator() {
    return new Iterator<BaseBufferEntry>() {
      int current = 0;
      final int last = count();

      @Override
      public boolean hasNext() {
        return current < last;
      }

      @Override
      public BaseBufferEntry next() {
        return new BaseBufferEntry(getKeyOffsetForBucketIndex(current++));
      }
    };
  }
}
