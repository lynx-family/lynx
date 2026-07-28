// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;

import com.lynx.tasm.base.AbsLogDelegate;
import com.lynx.tasm.base.LLog;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxLogContextTest {
  @Test
  public void formatsCanonicalTuple() {
    LynxLogContext context = new LynxLogContext(0, Integer.MAX_VALUE, -1);
    assertEquals("[0,2147483647,-1]", context.toString());
  }

  @Test
  public void contextualAndOrdinaryLoggerKeepExactPayloads() {
    AtomicReference<String> message = new AtomicReference<>();
    AbsLogDelegate delegate = new AbsLogDelegate() {
      @Override
      public void i(String tag, String value) {
        message.set(value);
      }
    };

    try {
      LLog.setDebugLoggingDelegate(delegate);
      LLog.i("tag", "ordinary");
      assertEquals("ordinary", message.get());

      LLog.i("tag", new LynxLogContext(1, 2, 3) + " contextual");
      assertEquals("[1,2,3] contextual", message.get());
    } finally {
      LLog.setDebugLoggingDelegate(null);
    }
  }
}
