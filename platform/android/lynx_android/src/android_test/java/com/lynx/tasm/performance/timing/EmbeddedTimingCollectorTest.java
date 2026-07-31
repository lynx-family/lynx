// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxViewClientGroupV2;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class EmbeddedTimingCollectorTest {
  @Test
  public void consumesReadyEventsWithoutClient() {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    LynxViewClientGroupV2 clients = new LynxViewClientGroupV2();
    collector.setObserver(new WeakReference<>(clients));

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming(TimingConstants.UPDATE_DATA_START, 1500);
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
    collector.markTiming(TimingConstants.PAINT_END, 2000);

    RecordingObserver observer = new RecordingObserver();
    clients.addClient(observer);
    collector.markTiming(TimingConstants.UPDATE_DATA_START, 3000);
    collector.markTiming(TimingConstants.PAINT_END, 4000);

    assertEquals(1, observer.mEntries.size());
    assertEquals(TimingConstants.UPDATE_TRIGGERED_BY_NATIVE, observer.mEntries.get(0).name);
  }

  @Test
  public void emitsLoadBundleWithObserver() {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    collector.setObserver(new WeakReference<>(observer));

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    collector.markTiming(TimingConstants.PAINT_END, 2000);

    assertEquals(1, observer.mEntries.size());
    assertEquals(TimingConstants.LOAD_BUNDLE, observer.mEntries.get(0).name);
  }

  @Test
  public void emitsLoadBundleWhenNativeTimingsArriveOutOfOrder() {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    collector.setObserver(new WeakReference<>(observer));

    collector.markTiming(TimingConstants.PAINT_END, 2000);
    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    assertTrue(observer.mEntries.isEmpty());

    collector.markTiming("loadBundleEnd", 2500);

    assertEquals(1, observer.mEntries.size());
    assertEquals(TimingConstants.LOAD_BUNDLE, observer.mEntries.get(0).name);
  }

  private static class RecordingObserver extends LynxViewClientV2 {
    private final List<PerformanceEntry> mEntries = new ArrayList<>();

    @Override
    public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
      mEntries.add(entry);
    }
  }
}
