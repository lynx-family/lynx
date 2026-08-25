// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxViewClientGroupV2;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
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

  private long mLoadBundleStartUs = -1;
  private long mLoadBundleEndUs = -1;
  private final ArrayList<Long> mUpdateDataStartUsList = new ArrayList<>();
  private long mPaintEndUs = -1;
  private boolean mHasEmitLoadBundleEvent = false;
  private boolean mHasReportedLoadBundleEvent = false;
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;

  private WeakReference<IPerformanceObserver> mObserver;

  /**
   * Set timing observer for event callbacks
   */
  public void setObserver(WeakReference<IPerformanceObserver> observer) {
    mObserver = observer;
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
        break;
      case LOAD_BUNDLE_END:
        mLoadBundleEndUs = usTimestamp;
        emitLoadBundleObserverIfReady(getObserver(), true);
        reportLoadBundleIfReady();
        break;
      case TimingConstants.UPDATE_DATA_START:
        mUpdateDataStartUsList.add(usTimestamp);
        break;
      case TimingConstants.PAINT_END:
        mPaintEndUs = usTimestamp;
        IPerformanceObserver observer = getObserver();
        emitLoadBundleObserverIfReady(observer, false);
        emitUpdateDataIfReady(observer);
        reportLoadBundleIfReady();
        break;
      default:
        // Ignore other timing points in embedded mode
        break;
    }
  }

  /** Notify the external observer after all load bundle timing points are available. */
  private void emitLoadBundleObserverIfReady(
      IPerformanceObserver observer, boolean dispatchOnReportThread) {
    if (mHasEmitLoadBundleEvent) {
      return;
    }
    if (mLoadBundleStartUs < 0 || mLoadBundleEndUs < mLoadBundleStartUs
        || mPaintEndUs < mLoadBundleEndUs) {
      return;
    }
    mHasEmitLoadBundleEvent = true;
    if (observer == null) {
      return;
    }

    final long loadBundleStartUs = mLoadBundleStartUs;
    final long loadBundleEndUs = mLoadBundleEndUs;
    final long paintEndUs = mPaintEndUs;
    if (dispatchOnReportThread) {
      WeakReference<IPerformanceObserver> observerRef = new WeakReference<>(observer);
      LynxEventReporter.runOnReportThread(() -> {
        IPerformanceObserver currentObserver = observerRef.get();
        if (currentObserver != null) {
          emitLoadBundleObserver(currentObserver, loadBundleStartUs, loadBundleEndUs, paintEndUs);
        }
      });
    } else {
      emitLoadBundleObserver(observer, loadBundleStartUs, loadBundleEndUs, paintEndUs);
    }
  }

  private static void emitLoadBundleObserver(IPerformanceObserver observer, long loadBundleStartUs,
      long loadBundleEndUs, long paintEndUs) {
    JavaOnlyMap entryMap = new JavaOnlyMap();
    entryMap.put("entryType", "pipeline");
    entryMap.put("name", TimingConstants.LOAD_BUNDLE);
    entryMap.put(TimingConstants.LOAD_BUNDLE_START, (double) loadBundleStartUs / 1000);
    entryMap.put(LOAD_BUNDLE_END, (double) loadBundleEndUs / 1000);
    entryMap.put(TimingConstants.PAINT_END, (double) paintEndUs / 1000);
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
    IPerformanceObserver observer = mObserver == null ? null : mObserver.get();
    if (observer instanceof LynxViewClientGroupV2
        && !((LynxViewClientGroupV2) observer).hasClients()) {
      return null;
    }
    return observer;
  }
}
