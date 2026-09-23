// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.memory;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;

import java.util.HashMap;
import org.junit.Test;

public class MemoryRecordTest {
  @Test
  public void copyCreatesDetachedSnapshot() {
    HashMap<String, String> detail = new HashMap<>();
    detail.put("url", "image-a");
    MemoryRecord record = new MemoryRecord("image", 50L, 2, detail);

    MemoryRecord copy = record.copy();
    record.mSizeBytes = 99L;
    record.mInstanceCount = 3;
    detail.put("url", "image-b");

    assertNotSame(record, copy);
    assertEquals("image", copy.getCategory());
    assertEquals(50L, copy.mSizeBytes);
    assertEquals(2, copy.mInstanceCount);
    assertNotSame(detail, copy.mDetail);
    assertEquals("image-a", copy.mDetail.get("url"));
  }

  @Test
  public void copyKeepsMissingDetailAbsent() {
    MemoryRecord nullDetail = new MemoryRecord("image", 50L, 2, null);
    MemoryRecord emptyDetail = new MemoryRecord("text", 12L, 3, new HashMap<>());

    assertNull(nullDetail.copy().mDetail);
    assertNull(emptyDetail.copy().mDetail);
  }
}
