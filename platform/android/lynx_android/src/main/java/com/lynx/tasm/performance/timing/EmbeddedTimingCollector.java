// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * @brief Embedded timing collector that provides minimal timing tracking for embedded mode,
 * tracking only the timing points needed by load and update pipelines.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class EmbeddedTimingCollector {
  private static final String LOAD_BUNDLE_END = "loadBundleEnd";
  private static final String PERFORMANCE_ENTRY_PIPELINE_EVENT =
      "lynxsdk_performance_entry_pipeline";
  private static final String LYNX_FCP = "lynxFcp";
  private static final String SETUP_TIMING = "setup_timing";
  private static final String EXTRA_TIMING = "extra_timing";
  private static final String UPDATE_TIMINGS = "update_timings";
  private static final String METRICS = "metrics";
  private static final String HAS_RELOAD = "has_reload";
  private static final String LOAD_BUNDLE_START_POLYFILL = "load_template_start";
  private static final String LOAD_BUNDLE_END_POLYFILL = "load_template_end";
  private static final String PAINT_END_POLYFILL = "draw_end";
  private static final String LYNX_FCP_POLYFILL = "lynx_fcp";

  private long mLoadBundleStartUs = -1;
  private long mLoadBundleEndUs = -1;
  private final ArrayList<Long> mUpdateDataStartUsList = new ArrayList<>();
  private long mPaintEndUs = -1;
  private boolean mHasEmitLoadBundleEvent = false;
  private boolean mHasEmitSetupTiming = false;
  private boolean mHasReportedLoadBundleEvent = false;
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;

  private WeakReference<IPerformanceObserver> mObserver;
  private WeakReference<LynxViewClient> mEmbeddedTimingClient;

  /**
   * Set timing observer for event callbacks
   */
  public void setObserver(WeakReference<IPerformanceObserver> observer) {
    mObserver = observer;
  }

  public void setEmbeddedTimingClient(WeakReference<LynxViewClient> timingClient) {
    mEmbeddedTimingClient = timingClient;
  }

  public void setInstanceId(int instanceId) {
    mInstanceId = instanceId;
  }

  public boolean hasEmitLoadBundleEvent() {
    return mHasEmitLoadBundleEvent;
  }

  public boolean hasPendingUpdateEvent() {
    return !mUpdateDataStartUsList.isEmpty();
  }

  public void markTiming(String key, long usTimestamp) {
    // Only track the essential timing points for embedded mode
    switch (key) {
      case TimingConstants.LOAD_BUNDLE_START:
        mLoadBundleStartUs = usTimestamp;
        emitSetupTimingIfReady();
        break;
      case LOAD_BUNDLE_END:
        mLoadBundleEndUs = usTimestamp;
        emitSetupTimingIfReady();
        emitLoadBundleObserverIfReady(getObserver());
        reportLoadBundleIfReady();
        break;
      case TimingConstants.UPDATE_DATA_START:
        mUpdateDataStartUsList.add(usTimestamp);
        break;
      case TimingConstants.PAINT_END:
        mPaintEndUs = usTimestamp;
        emitSetupTimingIfReady();
        IPerformanceObserver observer = getObserver();
        emitLoadBundleObserverIfReady(observer);
        emitUpdateDataIfReady(observer);
        reportLoadBundleIfReady();
        break;
      default:
        // Ignore other timing points in embedded mode
        break;
    }
  }

  private void emitSetupTimingIfReady() {
    if (mHasEmitSetupTiming) {
      return;
    }
    final WeakReference<LynxViewClient> timingClient = mEmbeddedTimingClient;
    if (timingClient == null || timingClient.get() == null) {
      return;
    }
    if (mLoadBundleStartUs < 0 || mLoadBundleEndUs < mLoadBundleStartUs
        || mPaintEndUs < mLoadBundleEndUs) {
      return;
    }
    mHasEmitSetupTiming = true;

    final long loadBundleStartUs = mLoadBundleStartUs;
    final long loadBundleEndUs = mLoadBundleEndUs;
    final long paintEndUs = mPaintEndUs;
    UIThreadUtils.runOnUiThread(() -> {
      LynxViewClient client = timingClient.get();
      if (client == null) {
        return;
      }

      JavaOnlyMap setupTiming = new JavaOnlyMap();
      setupTiming.putDouble(LOAD_BUNDLE_START_POLYFILL, (double) loadBundleStartUs / 1000);
      setupTiming.putDouble(LOAD_BUNDLE_END_POLYFILL, (double) loadBundleEndUs / 1000);
      setupTiming.putDouble(PAINT_END_POLYFILL, (double) paintEndUs / 1000);

      JavaOnlyMap metrics = new JavaOnlyMap();
      metrics.putDouble(LYNX_FCP_POLYFILL, (double) (paintEndUs - loadBundleStartUs) / 1000);

      JavaOnlyMap timingInfo = new JavaOnlyMap();
      timingInfo.putMap(SETUP_TIMING, setupTiming);
      timingInfo.putMap(EXTRA_TIMING, new JavaOnlyMap());
      timingInfo.putMap(UPDATE_TIMINGS, new JavaOnlyMap());
      timingInfo.putMap(METRICS, metrics);
      timingInfo.putBoolean(HAS_RELOAD, false);
      client.onTimingSetup(timingInfo.asHashMap());
    });
  }

  /** Notify the external observer after the first paint, preserving the existing contract. */
  private void emitLoadBundleObserverIfReady(IPerformanceObserver observer) {
    if (mHasEmitLoadBundleEvent) {
      return;
    }
    if (mLoadBundleStartUs < 0 || mPaintEndUs < mLoadBundleStartUs) {
      return;
    }
    mHasEmitLoadBundleEvent = true;
    if (observer == null) {
      return;
    }

    JavaOnlyMap entryMap = new JavaOnlyMap();
    entryMap.put("entryType", "pipeline");
    entryMap.put("name", TimingConstants.LOAD_BUNDLE);
    entryMap.put(TimingConstants.LOAD_BUNDLE_START, (double) mLoadBundleStartUs / 1000);
    entryMap.put(TimingConstants.PAINT_END, (double) mPaintEndUs / 1000);
    observer.onPerformanceEvent(PerformanceEntryConverter.makePerformanceEntry(entryMap));
  }

  private void reportLoadBundleIfReady() {
    if (mHasReportedLoadBundleEvent || mLoadBundleStartUs < 0
        || mLoadBundleEndUs < mLoadBundleStartUs || mPaintEndUs < mLoadBundleEndUs) {
      return;
    }
    mHasReportedLoadBundleEvent = true;

    final long loadBundleStartUs = mLoadBundleStartUs;
    final long loadBundleEndUs = mLoadBundleEndUs;
    final long paintEndUs = mPaintEndUs;
    final int instanceId = mInstanceId;
    LynxEventReporter.onEvent(PERFORMANCE_ENTRY_PIPELINE_EVENT, instanceId, () -> {
      HashMap<String, Object> props = new HashMap<>();
      props.put("entryType", "pipeline");
      props.put("name", TimingConstants.LOAD_BUNDLE);
      props.put(LYNX_FCP, (double) (paintEndUs - loadBundleStartUs) / 1000);
      props.put(TimingConstants.LOAD_BUNDLE, (double) (loadBundleEndUs - loadBundleStartUs) / 1000);
      return props;
    });
  }

  private void emitUpdateDataIfReady(IPerformanceObserver observer) {
    if (observer == null) {
      mUpdateDataStartUsList.clear();
      return;
    }

    ArrayList<Long> updateDataStartUsList = new ArrayList<>(mUpdateDataStartUsList);
    mUpdateDataStartUsList.clear();
    for (Long updateDataStartUs : updateDataStartUsList) {
      JavaOnlyMap entryMap = new JavaOnlyMap();
      entryMap.put("entryType", "pipeline");
      entryMap.put("name", TimingConstants.UPDATE_TRIGGERED_BY_NATIVE);
      entryMap.put(TimingConstants.PIPELINE_START, (double) updateDataStartUs / 1000);
      entryMap.put(TimingConstants.PAINT_END, (double) mPaintEndUs / 1000);

      PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
      observer.onPerformanceEvent(entry);
    }
  }

  private IPerformanceObserver getObserver() {
    return mObserver == null ? null : mObserver.get();
  }
}
