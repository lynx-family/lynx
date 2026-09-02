// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.Test;
import org.mockito.InOrder;

public class FSPReportDispatcherTest {
  @Test
  public void nativeDispatcherPreservesFifoAndReportThreadTaskOrder() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    FSPReportProcessor processor = mock(FSPReportProcessor.class);
    final int snapshotCount = 256;
    MeaningfulContentSnapshot[] snapshots = new MeaningfulContentSnapshot[snapshotCount];
    for (int i = 0; i < snapshotCount; i++) {
      snapshots[i] = mock(MeaningfulContentSnapshot.class);
    }
    CountDownLatch snapshotsProcessed = new CountDownLatch(snapshotCount);
    doAnswer(invocation -> {
      snapshotsProcessed.countDown();
      return null;
    })
        .when(processor)
        .processSnapshotOnReportThread(
            org.mockito.ArgumentMatchers.any(MeaningfulContentSnapshot.class),
            org.mockito.ArgumentMatchers.anyLong());

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

    FSPReportDispatcher dispatcher = new FSPReportDispatcher(processor);
    AtomicBoolean markerObservedAllSnapshots = new AtomicBoolean(false);
    CountDownLatch markerRan = new CountDownLatch(1);
    try {
      for (int i = 0; i < snapshotCount; i++) {
        assertTrue(dispatcher.post(snapshots[i], i));
      }
      // This marker models a hard-timeout task posted after the captured frames. Every frame task
      // must already be in the native report-thread queue, so the marker cannot overtake them.
      LynxEventReporter.runOnReportThread(() -> {
        markerObservedAllSnapshots.set(snapshotsProcessed.getCount() == 0);
        markerRan.countDown();
      });
    } finally {
      releaseBlocker.countDown();
    }

    assertTrue(markerRan.await(5, TimeUnit.SECONDS));
    assertTrue(markerObservedAllSnapshots.get());
    assertTrue(snapshotsProcessed.await(5, TimeUnit.SECONDS));
    dispatcher.close();
    assertFalse(dispatcher.post(snapshots[0], snapshotCount));

    InOrder inOrder = inOrder(processor);
    for (int i = 0; i < snapshotCount; i++) {
      inOrder.verify(processor).processSnapshotOnReportThread(snapshots[i], i);
    }
  }

  @Test
  public void closeDropsPendingSnapshots() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    FSPReportProcessor processor = mock(FSPReportProcessor.class);
    MeaningfulContentSnapshot snapshot = mock(MeaningfulContentSnapshot.class);

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

    FSPReportDispatcher dispatcher = new FSPReportDispatcher(processor);
    assertTrue(dispatcher.post(snapshot, 1));
    dispatcher.close();
    releaseBlocker.countDown();

    CountDownLatch reportThreadDrained = new CountDownLatch(1);
    LynxEventReporter.runOnReportThread(reportThreadDrained::countDown);
    assertTrue(reportThreadDrained.await(5, TimeUnit.SECONDS));
    verify(processor, never()).processSnapshotOnReportThread(snapshot, 1);
  }
}
