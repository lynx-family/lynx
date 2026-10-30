// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.Test;
import org.mockito.InOrder;

public class FSPReportDispatcherTest {
  @Test
  public void realNativeDispatcherKeepsOneGlobalRefUntilBacklogDrains() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    int baseline = FSPReportDispatcherTestHelper.getActiveGlobalRefCount();
    FSPTracer tracer = mock(FSPTracer.class);
    final int snapshotCount = 256;
    MeaningfulContentSnapshot[] snapshots = new MeaningfulContentSnapshot[snapshotCount];
    for (int i = 0; i < snapshotCount; i++) {
      snapshots[i] = mock(MeaningfulContentSnapshot.class);
    }

    CountDownLatch blockerStarted = new CountDownLatch(1);
    CountDownLatch releaseBlocker = new CountDownLatch(1);
    LynxEventReporter.runOnReportThread(() -> {
      blockerStarted.countDown();
      try {
        releaseBlocker.await(30, TimeUnit.SECONDS);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    });
    assertTrue(blockerStarted.await(5, TimeUnit.SECONDS));

    FSPReportDispatcher dispatcher = new FSPReportDispatcher(tracer);
    try {
      assertEquals(baseline + 1, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());
      for (int i = 0; i < snapshotCount; i++) {
        assertTrue(dispatcher.post(snapshots[i], i));
      }
      assertEquals(baseline + 1, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());

      dispatcher.close();
      dispatcher.close();
      assertFalse(dispatcher.post(snapshots[0], snapshotCount));
      assertEquals(baseline + 1, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());
    } finally {
      dispatcher.close();
      releaseBlocker.countDown();
    }

    CountDownLatch backlogDrained = new CountDownLatch(1);
    LynxEventReporter.runOnReportThread(backlogDrained::countDown);
    assertTrue(backlogDrained.await(5, TimeUnit.SECONDS));
    assertEquals(baseline, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());
    InOrder inOrder = inOrder(tracer);
    for (int i = 0; i < snapshotCount; i++) {
      inOrder.verify(tracer).processSnapshotOnReportThread(snapshots[i], i);
    }

    for (int i = 0; i < 100; i++) {
      FSPReportDispatcher reloadDispatcher = new FSPReportDispatcher(tracer);
      assertEquals(baseline + 1, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());
      reloadDispatcher.close();
      assertEquals(baseline, FSPReportDispatcherTestHelper.getActiveGlobalRefCount());
    }
  }
}
