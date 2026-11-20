// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.base.LynxConsumer;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class LynxJNIHeapSnapshotTest {
  @Test
  public void dispatchesResultsThroughStableJniSignature() throws Exception {
    // Resolve the same parameter types as native code, without naming LynxConsumer.
    Method dispatch =
        LynxJNI.class.getDeclaredMethod("dispatchHeapSnapshotResult", Object.class, String.class);
    assertTrue(Modifier.isStatic(dispatch.getModifiers()));
    assertEquals(void.class, dispatch.getReturnType());

    List<String> results = new ArrayList<>();
    LynxConsumer<String> callback = results::add;
    dispatch.invoke(null, callback, null);
    dispatch.invoke(null, callback, "Write failed");
    assertEquals(2, results.size());
    assertNull(results.get(0));
    assertEquals("Write failed", results.get(1));

    dispatch.invoke(null, null, null);
    dispatch.invoke(null, null, "Cancelled");
    assertEquals(2, results.size());
  }
}
