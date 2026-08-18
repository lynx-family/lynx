// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.LynxViewClientGroupV2;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
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

  @Test
  public void dispatchesSetupTimingWhenPaintEnds() throws Exception {
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    RecordingObserver observer = new RecordingObserver();
    RecordingTimingClient timingClient = new RecordingTimingClient();
    collector.setObserver(new WeakReference<>(observer));
    collector.setTimingClient(new WeakReference<>(timingClient));

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1000);
    assertTrue(timingClient.mSetupTimingEvents.isEmpty());
    assertFalse(collector.hasEmitLoadBundleEvent());

    collector.markTiming(TimingConstants.PAINT_END, 3000);

    assertTrue(timingClient.mSetupTimingLatch.await(5, TimeUnit.SECONDS));
    assertEquals(1, timingClient.mSetupTimingEvents.size());
    Map<String, Object> timingInfo = timingClient.mSetupTimingEvents.get(0);
    JavaOnlyMap setupTiming = (JavaOnlyMap) timingInfo.get("setup_timing");
    JavaOnlyMap metrics = (JavaOnlyMap) timingInfo.get("metrics");
    assertNotNull(setupTiming);
    assertNotNull(metrics);
    assertEquals(1.0, (Double) setupTiming.get("load_template_start"), 0.0);
    assertEquals(3.0, (Double) setupTiming.get("draw_end"), 0.0);
    assertEquals(2.0, (Double) metrics.get("lynx_fcp"), 0.0);
    assertEquals(false, timingInfo.get("has_reload"));
    assertEquals(1, observer.mEntries.size());
    assertTrue(collector.hasEmitLoadBundleEvent());

    collector.markTiming(TimingConstants.PAINT_END, 4000);
    assertEquals(1, timingClient.mSetupTimingEvents.size());
  }

  private static class RecordingObserver extends LynxViewClientV2 {
    private final List<PerformanceEntry> mEntries = new ArrayList<>();

    @Override
    public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
      mEntries.add(entry);
    }
  }

  private static class RecordingTimingClient extends LynxViewClient {
    private final List<Map<String, Object>> mSetupTimingEvents = new ArrayList<>();
    private final CountDownLatch mSetupTimingLatch = new CountDownLatch(1);

    @Override
    public void onTimingSetup(Map<String, Object> timingInfo) {
      mSetupTimingEvents.add(timingInfo);
      mSetupTimingLatch.countDown();
    }
  }
}
