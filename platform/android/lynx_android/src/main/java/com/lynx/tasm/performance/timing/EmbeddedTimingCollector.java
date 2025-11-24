// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import java.lang.ref.WeakReference;

/**
 * @brief Embedded timing collector that provides minimal timing tracking
 * for embedded mode, tracking only the essential timing points (loadBundleStart,
 * loadBundleEnd, DrawEnd).
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class EmbeddedTimingCollector {
  private long mLoadBundleStartUs;
  private long mPaintEndUs;
  private boolean hasEmitTimingEvent = false;

  private WeakReference<IPerformanceObserver> mObserver;

  /**
   * Set timing observer for event callbacks
   */
  public void setObserver(WeakReference<IPerformanceObserver> observer) {
    mObserver = observer;
  }

  public boolean hasEmitTimingEvent() {
    return hasEmitTimingEvent;
  }

  public void markTiming(String key, long usTimestamp) {
    // Only track the essential timing points for embedded mode
    switch (key) {
      case TimingConstants.LOAD_BUNDLE_START:
        mLoadBundleStartUs = usTimestamp;
        break;
      case TimingConstants.PAINT_END:
        mPaintEndUs = usTimestamp;
        emitIfReady();
        break;
      default:
        // Ignore other timing points in embedded mode
        break;
    }
  }

  /**
   * Emit timing event if all required data is available
   */
  private void emitIfReady() {
    if (hasEmitTimingEvent) {
      return;
    }
    if (mObserver == null) {
      return;
    }
    hasEmitTimingEvent = true;

    JavaOnlyMap entryMap = new JavaOnlyMap();
    entryMap.put("entryType", "pipeline");
    entryMap.put("name", TimingConstants.LOAD_BUNDLE);
    entryMap.put(TimingConstants.LOAD_BUNDLE_START, (double) mLoadBundleStartUs / 1000);
    entryMap.put(TimingConstants.PAINT_END, (double) mPaintEndUs / 1000);

    IPerformanceObserver observer = mObserver.get();
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (observer != null) {
      observer.onPerformanceEvent(entry);
    }
  }
}
