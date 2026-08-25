// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxViewClientGroupV2;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class EmbeddedTimingCollectorTest {
  @Test
  public void consumesReadyEventsWithoutClient() {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    LynxViewClientGroupV2 clients = new LynxViewClientGroupV2();
    collector.setObserver(new WeakReference<>(clients));

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming(TimingConstants.UPDATE_DATA_START, 1500);
    collector.markTiming("loadBundleEnd", 1750);
    collector.markTiming(TimingConstants.PAINT_END, 2000);

    assertTrue(collector.hasEmitLoadBundleEvent());
    assertFalse(collector.hasPendingUpdateEvent());
  }

  @Test
  public void emitsFutureUpdateAfterClientIsAttached() {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    LynxViewClientGroupV2 clients = new LynxViewClientGroupV2();
    collector.setObserver(new WeakReference<>(clients));
    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming("loadBundleEnd", 1500);
    collector.markTiming(TimingConstants.PAINT_END, 2000);

    RecordingObserver observer = new RecordingObserver();
    clients.addClient(observer);
    collector.markTiming(TimingConstants.UPDATE_DATA_START, 3000);
    collector.markTiming(TimingConstants.PAINT_END, 4000);

    assertEquals(1, observer.mEntries.size());
    assertEquals(TimingConstants.UPDATE_TRIGGERED_BY_NATIVE, observer.mEntries.get(0).name);
  }

  @Test
  public void emitsLoadBundleSynchronouslyForNormalTimingOrder() throws Exception {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    collector.setObserver(new WeakReference<>(observer));
    Thread callingThread = Thread.currentThread();

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming("loadBundleEnd", 1500);
    assertTrue(observer.mEntries.isEmpty());

    collector.markTiming(TimingConstants.PAINT_END, 2000);

    assertEquals(1, observer.mEntries.size());
    assertSame(callingThread, observer.mThread.get());
    assertEquals(TimingConstants.LOAD_BUNDLE, observer.mEntries.get(0).name);
  }

  @Test
  public void emitsLoadBundleWithObserver() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    collector.setObserver(new WeakReference<>(observer));
    Thread callingThread = Thread.currentThread();

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming(TimingConstants.PAINT_END, 2000);
    assertTrue(observer.mEntries.isEmpty());

    collector.markTiming("loadBundleEnd", 1500);
    assertTrue(observer.mEntryLatch.await(5, TimeUnit.SECONDS));
    assertEquals(1, observer.mEntries.size());
    assertNotSame(callingThread, observer.mThread.get());
    assertEquals(TimingConstants.LOAD_BUNDLE, observer.mEntries.get(0).name);
    assertEquals(1.5,
        ((Number) observer.mEntries.get(0).toHashMap().get("loadBundleEnd")).doubleValue(), 0.0);
  }

  @Test
  public void emitsLoadBundleWhenNativeTimingsArriveOutOfOrder() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    collector.setObserver(new WeakReference<>(observer));

    collector.markTiming(TimingConstants.PAINT_END, 2000);
    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    assertTrue(observer.mEntries.isEmpty());

    collector.markTiming("loadBundleEnd", 1500);

    assertTrue(observer.mEntryLatch.await(5, TimeUnit.SECONDS));
    assertEquals(1, observer.mEntries.size());
    assertEquals(TimingConstants.LOAD_BUNDLE, observer.mEntries.get(0).name);
    assertEquals(1.5,
        ((Number) observer.mEntries.get(0).toHashMap().get("loadBundleEnd")).doubleValue(), 0.0);
  }

  private static class RecordingObserver extends LynxViewClientV2 {
    private final List<PerformanceEntry> mEntries = new ArrayList<>();
    private final CountDownLatch mEntryLatch = new CountDownLatch(1);
    private final AtomicReference<Thread> mThread = new AtomicReference<>();

    @Override
    public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
      mEntries.add(entry);
      mThread.set(Thread.currentThread());
      mEntryLatch.countDown();
    }
  }
}
