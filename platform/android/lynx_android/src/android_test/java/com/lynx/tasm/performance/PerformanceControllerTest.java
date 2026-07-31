// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.eventreport.ILynxEventReportObserver;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.fsp.FSPTracer;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.timing.EmbeddedTimingCollector;
import com.lynx.tasm.performance.timing.TimingConstants;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Before;
import org.junit.Test;

public class PerformanceControllerTest {
  private PerformanceController performanceController;
  private IPerformanceObserver strongPerformanceObserver;

  @Before
  public void setUp() {
    performanceController = spy(new PerformanceController());
  }

  @Test
  public void testSetExtraTiming() throws Exception {
    TimingHandler.ExtraTimingInfo extraTimingInfo = new TimingHandler.ExtraTimingInfo();
    extraTimingInfo.mOpenTime = 1000;
    extraTimingInfo.mContainerInitStart = 2000;
    extraTimingInfo.mContainerInitEnd = 3000;
    extraTimingInfo.mPrepareTemplateStart = 4000;
    extraTimingInfo.mPrepareTemplateEnd = 5000;

    performanceController.setExtraTiming(extraTimingInfo);
  }

  @Test
  public void embeddedTimingCollectorEmitsBasicLoadBundleMetricsOnce() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    EmbeddedTimingCollector collector = new EmbeddedTimingCollector();
    AtomicReference<PerformanceEntry> receivedEntry = new AtomicReference<>();
    Thread callingThread = Thread.currentThread();
    AtomicReference<Thread> observerThread = new AtomicReference<>();
    AtomicInteger eventCount = new AtomicInteger();
    CountDownLatch entryLatch = new CountDownLatch(1);
    strongPerformanceObserver = entry -> {
      receivedEntry.set(entry);
      observerThread.set(Thread.currentThread());
      eventCount.incrementAndGet();
      entryLatch.countDown();
    };
    collector.setObserver(new WeakReference<>(strongPerformanceObserver));

    collector.markTiming(TimingConstants.LOAD_BUNDLE_START, 1_000_000);
    assertNull(receivedEntry.get());

    collector.markTiming(TimingConstants.PAINT_END, 1_100_000);
    assertSame(callingThread, observerThread.get());
    assertTrue(entryLatch.await(5, TimeUnit.SECONDS));
    assertTrue(collector.hasEmitLoadBundleEvent());

    PerformanceEntry entry = receivedEntry.get();
    assertNotNull(entry);
    assertEquals("pipeline", entry.entryType);
    assertEquals(TimingConstants.LOAD_BUNDLE, entry.name);
    assertEquals(1000.0, ((Number) entry.toHashMap().get("loadBundleStart")).doubleValue(), 0.0);
    assertFalse(entry.toHashMap().containsKey("loadBundleEnd"));
    assertEquals(
        1100.0, ((Number) entry.toHashMap().get(TimingConstants.PAINT_END)).doubleValue(), 0.0);

    collector.markTiming("loadBundleEnd", 1_040_000);
    collector.markTiming(TimingConstants.PAINT_END, 1_200_000);
    CountDownLatch drainLatch = new CountDownLatch(1);
    LynxEventReporter.runOnReportThread(drainLatch::countDown);
    assertTrue(drainLatch.await(5, TimeUnit.SECONDS));
    assertEquals(1, eventCount.get());
  }

  @Test
  public void embeddedTimingReportsAfterAllMetricsOnReportThread() throws Exception {
    assertTrue(LynxEnv.inst().initNativeLibraries(System::loadLibrary));
    PerformanceController embeddedController = new PerformanceController();
    int instanceId = 314;
    CountDownLatch entryLatch = new CountDownLatch(1);
    CountDownLatch eventLatch = new CountDownLatch(1);
    AtomicInteger observerCount = new AtomicInteger();
    strongPerformanceObserver = entry -> {
      observerCount.incrementAndGet();
      entryLatch.countDown();
    };
    AtomicInteger reportCount = new AtomicInteger();
    AtomicReference<Map<String, ? extends Object>> receivedProps = new AtomicReference<>();
    AtomicReference<Thread> reportThread = new AtomicReference<>();
    ILynxEventReportObserver eventObserver = new ILynxEventReportObserver() {
      @Override
      public void onReportEvent(@NonNull String eventName, int reportedInstanceId,
          @NonNull Map<String, ? extends Object> props,
          @Nullable Map<String, ? extends Object> extraData) {
        if ("lynxsdk_performance_entry_pipeline".equals(eventName)
            && reportedInstanceId == instanceId) {
          reportCount.incrementAndGet();
          receivedProps.set(props);
          reportThread.set(Thread.currentThread());
          eventLatch.countDown();
        }
      }
    };
    LynxEventReporter.addObserver(eventObserver);
    try {
      embeddedController.setEmbeddedMode(true);
      embeddedController.setInstanceId(instanceId);
      embeddedController.setPerformanceObserver(strongPerformanceObserver);
      embeddedController.setEmbeddedTiming(
          TimingConstants.LOAD_BUNDLE_START, 2_000_000, "pipeline-id");
      embeddedController.setEmbeddedTiming(TimingConstants.PAINT_END, 2_100_000, "pipeline-id");

      assertEquals(0, entryLatch.getCount());
      assertTrue(entryLatch.await(5, TimeUnit.SECONDS));
      assertEquals(1, eventLatch.getCount());

      // Ignore later draws once the observer event has been emitted and no update is pending.
      // The native loadBundleEnd timing may arrive later, but its timestamp is before paintEnd.
      embeddedController.markPaintEndTimingIfNeeded();
      embeddedController.setEmbeddedTiming("loadBundleEnd", 2_040_123, "pipeline-id");

      assertTrue(eventLatch.await(5, TimeUnit.SECONDS));
      assertNotSame(Looper.getMainLooper().getThread(), reportThread.get());
      Map<String, ? extends Object> props = receivedProps.get();
      assertEquals("pipeline", props.get("entryType"));
      assertEquals("loadBundle", props.get("name"));
      assertEquals(40.123, ((Number) props.get("loadBundle")).doubleValue(), 0.0001);
      assertEquals(100.0, ((Number) props.get("lynxFcp")).doubleValue(), 0.0001);
      assertEquals(1, observerCount.get());

      embeddedController.setEmbeddedTiming("loadBundleEnd", 2_050_000, "pipeline-id");
      CountDownLatch drainLatch = new CountDownLatch(1);
      LynxEventReporter.runOnReportThread(drainLatch::countDown);
      assertTrue(drainLatch.await(5, TimeUnit.SECONDS));
      assertEquals(1, reportCount.get());
    } finally {
      LynxEventReporter.removeObserver(eventObserver);
    }
  }

  @Test
  public void startFSPTracerCreatesTracer() throws Exception {
    setEnableFSPField(true);

    performanceController.startFSPTracer(() -> null);

    assertNotNull(getFSPTracer());
  }

  @Test
  public void startFSPTracerUsesPageConfig() throws Exception {
    setEnableFSPField(false);
    performanceController.onPageConfigDecoded(createPageConfig(true));

    performanceController.startFSPTracer(() -> null);

    assertNotNull(getFSPTracer());
  }

  @Test
  public void startFSPTracerStopsForDisabledPageConfig() throws Exception {
    setEnableFSPField(true);
    performanceController.onPageConfigDecoded(createPageConfig(false));

    performanceController.startFSPTracer(() -> null);

    assertNull(getFSPTracer());
  }

  @Test
  public void stopFSPTracerByUserInteractionUsesPageConfig() throws Exception {
    FSPTracer fspTracer = mock(FSPTracer.class);
    setField(PerformanceController.class, performanceController, "mFSPTracer", fspTracer);

    performanceController.onPageConfigDecoded(createPageConfig(false));
    performanceController.stopFSPTracerByUserInteraction();
    verify(fspTracer, never()).cancelledByUserInteraction();

    performanceController.onPageConfigDecoded(createPageConfig(true));
    performanceController.stopFSPTracerByUserInteraction();
    verify(fspTracer).cancelledByUserInteraction();
  }

  private FSPTracer getFSPTracer() throws Exception {
    return (FSPTracer) getField(PerformanceController.class, performanceController, "mFSPTracer");
  }

  private PageConfig createPageConfig(boolean enableFSP) {
    JavaOnlyMap map = new JavaOnlyMap();
    map.putBoolean("enableFSP", enableFSP);
    return new PageConfig(map);
  }

  private void setEnableFSPField(boolean value) throws Exception {
    Field field = LynxEnv.class.getDeclaredField("mEnableFSP");
    field.setAccessible(true);
    field.set(LynxEnv.inst(), value);
  }

  private Object getField(Class clazz, Object target, String fieldName) throws Exception {
    Field field = clazz.getDeclaredField(fieldName);
    field.setAccessible(true);
    return field.get(target);
  }

  private void setField(Class clazz, Object target, String fieldName, Object value)
      throws Exception {
    Field field = clazz.getDeclaredField(fieldName);
    field.setAccessible(true);
    field.set(target, value);
  }
}
