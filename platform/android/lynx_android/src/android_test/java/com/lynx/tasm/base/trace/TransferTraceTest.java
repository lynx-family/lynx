// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base.trace;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.base.TraceController;
import com.lynx.tasm.base.TraceEvent;
import java.lang.reflect.Field;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class TransferTraceTest {
  @Test
  public void inactiveTracingSkipsSections() {
    TransferTrace.begin("Transfer.Inactive", 7, "transfer-1", 3);
    TransferTrace.end("Transfer.Inactive");
    TransferTrace.begin("Transfer.Inactive", -1, null, -1);
    TransferTrace.end("Transfer.Inactive");
  }

  @Test
  public void activeTracingRecordsOptionalCorrelation() throws Exception {
    Field tracingStarted = TraceController.class.getDeclaredField("mTracingStarted");
    tracingStarted.setAccessible(true);
    boolean wasStarted = tracingStarted.getBoolean(null);
    TraceEvent.markTraceDebugMode(true);
    try {
      tracingStarted.setBoolean(null, true);
      TransferTrace.begin("Transfer.Active", 42, "transfer-42", 7);
      TransferTrace.end("Transfer.Active");
      TransferTrace.begin("Transfer.Active", 42, null, -1);
      TransferTrace.end("Transfer.Active");
    } finally {
      tracingStarted.setBoolean(null, wasStarted);
      TraceEvent.markTraceDebugMode(false);
    }
  }
}
