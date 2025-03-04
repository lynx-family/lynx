// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

public class GlobalRefQueue {
  private final ConcurrentHashMap<Long, Object> mRefs = new ConcurrentHashMap<>();
  private final AtomicLong mIndex = new AtomicLong(0L);

  public GlobalRefQueue() {}

  public long push(Object ref) {
    if (ref == null) {
      return -1;
    }
    long index = mIndex.incrementAndGet();
    mRefs.put(index, ref);
    return index;
  }

  public Object pop(long index) {
    if (index < 0) {
      return null;
    }
    return mRefs.remove(index);
  }
}
