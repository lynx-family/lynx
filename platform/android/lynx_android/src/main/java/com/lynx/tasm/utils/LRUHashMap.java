// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import java.util.LinkedHashMap;
import java.util.Map;

public class LRUHashMap<K, V> extends LinkedHashMap<K, V> {
  private final int capacity;

  public LRUHashMap(int capacity) {
    super(capacity, 0.75f, true);
    this.capacity = capacity;
  }

  @Override
  protected boolean removeEldestEntry(Map.Entry<K, V> eldest) {
    return size() > capacity;
  }

  public int getCapacity() {
    return capacity;
  }
}
