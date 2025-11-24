// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static com.lynx.tasm.base.trace.TraceEventDef.INSTANCE_ID;
import static com.lynx.tasm.base.trace.TraceEventDef.MARK_HOST_PLATFORM_TIMING;
import static com.lynx.tasm.base.trace.TraceEventDef.MARK_TIMING;
import static com.lynx.tasm.base.trace.TraceEventDef.PIPELINE_ID;
import static com.lynx.tasm.base.trace.TraceEventDef.TIMING_KEY;
import static com.lynx.tasm.base.trace.TraceEventDef.TIMING_KEY_PAINT_END;
import static com.lynx.tasm.base.trace.TraceEventDef.TIMING_TIMESTAMP;

import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.fsp.FSPTracer;
import com.lynx.tasm.performance.fsp.IMeaningfulContentSnapshotCaptureHandler;
import com.lynx.tasm.performance.memory.IMemoryMonitor;
import com.lynx.tasm.performance.memory.IMemoryRecordBuilder;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.performance.timing.EmbeddedTimingCollector;
import com.lynx.tasm.performance.timing.ITimingCollector;
import com.lynx.tasm.service.ILynxEventReporterService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

/**
 * @brief Manages performance data collection and observation. This class acts as a central point
 * for performance-related events, conforming to timing collection, memory monitoring, and
 * performance observation protocols.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class PerformanceController implements IMemoryMonitor, ITimingCollector {
  private static final String TAG = "PerformanceController";
  private static volatile boolean sIsNativeLibraryLoaded = false;
  private static volatile LynxBooleanOption sIsMemoryMonitorEnabled = LynxBooleanOption.UNSET;
  private static volatile long sMemoryAcquisitionDelaySec = -1;
  private EmbeddedTimingCollector mEmbeddedTimingCollector;
  private volatile long mNativePerformanceActorPtr = 0;
  private WeakReference<IPerformanceObserver> mObserver;
  private WeakReference<ILynxEventReporterService> mEventReporterService;
  private boolean mUseEmbeddedMode = false;
  private JavaOnlyMap mHostPlatformTiming;
  private JavaOnlyArray mPendingPaintEndPipelineIds = new JavaOnlyArray();
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
  private FSPTracer mFSPTracer = null;

  /**
   * Set embedded mode
   * @param useEmbeddedMode true to use embedded mode, false for standard mode
   */
  public void setEmbeddedMode(boolean useEmbeddedMode) {
    mUseEmbeddedMode = useEmbeddedMode;
  }

  public boolean isEmbeddedMode() {
    return mUseEmbeddedMode;
  }

  /**
   * Ensure embedded collector is initialized if in embedded mode
   */
  private void ensureEmbeddedCollectorInitialized() {
    if (isEmbeddedMode() && mEmbeddedTimingCollector == null) {
      mEmbeddedTimingCollector = new EmbeddedTimingCollector();
      mEmbeddedTimingCollector.setObserver(mObserver);
    }
  }

  /**
   * Checks if memory monitoring is enabled.
   * Modules can call this before collecting data to avoid unnecessary collection.
   */
  public static boolean isMemoryMonitorEnabled() {
    if (isNativeLibraryLoaded()) {
      LynxBooleanOption op = sIsMemoryMonitorEnabled;
      if (op == LynxBooleanOption.FALSE) {
        return false;
      } else if (op == LynxBooleanOption.UNSET) {
        boolean ret = nativeIsMemoryMonitorEnabled();
        sIsMemoryMonitorEnabled = ret ? LynxBooleanOption.TRUE : LynxBooleanOption.FALSE;
        return ret;
      }
      return true;
    }
    return false;
  }

  public static long getMemoryAcquisitionDelaySec() {
    if (sMemoryAcquisitionDelaySec >= 0) {
      return sMemoryAcquisitionDelaySec;
    }
    String value = LynxEnv.inst().getMemoryAcquisitionDelaySec();
    // default is 2 second.
    long delay = 2;
    if (value != null && !value.isEmpty()) {
      try {
        delay = Long.parseLong(value);
        sMemoryAcquisitionDelaySec = delay;
      } catch (NumberFormatException ignored) {
      }
    }
    return delay;
  }

  public void setPerformanceObserver(IPerformanceObserver observer) {
    mObserver = new WeakReference<>(observer);
    if (mEmbeddedTimingCollector != null) {
      mEmbeddedTimingCollector.setObserver(mObserver);
    }
  }

  public void setInstanceId(int instanceId) {
    mInstanceId = instanceId;
  }

  @Override
  public void allocateMemory(IMemoryRecordBuilder builder) {
    if (isEmbeddedMode()) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeAllocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.mSizeBytes, null, null);
    });
  }

  @Override
  public void deallocateMemory(IMemoryRecordBuilder builder) {
    if (isEmbeddedMode()) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeDeallocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.mSizeBytes, null, null);
    });
  }

  @Override
  public void updateMemoryUsage(IMemoryRecordBuilder builder) {
    if (isEmbeddedMode()) {
      return;
    }
    if (builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeUpdateMemoryUsage(mNativePerformanceActorPtr, record.getCategory(), record.mSizeBytes,
          record.mInstanceCount, null);
    });
  }

  @UiThread
  public void startFSPTracer(IMeaningfulContentSnapshotCaptureHandler captureHandler) {
    if (!LynxEnv.inst().enableFSP()) {
      return;
    }
    if (mFSPTracer == null) {
      mFSPTracer = new FSPTracer(this);
    }
    mFSPTracer.start(captureHandler);
  }

  @UiThread
  public void stopFSPTracerByUserInteraction() {
    if (!LynxEnv.inst().enableFSP() || mFSPTracer == null) {
      return;
    }
    mFSPTracer.cancelledByUserInteraction();
  }

  @Override
  public void updateMemoryUsage(Map<String, MemoryRecord> recordMap) {
    if (isEmbeddedMode() || recordMap == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      for (Map.Entry<String, MemoryRecord> recordEntry : recordMap.entrySet()) {
        MemoryRecord record = recordEntry.getValue();
        if (record == null) {
          continue;
        }
        nativeUpdateMemoryUsage(mNativePerformanceActorPtr, record.getCategory(), record.mSizeBytes,
            record.mInstanceCount, record.mDetail);
      }
    });
  }

  @Override
  public void setMsTiming(String key, long msTimestamp, String pipelineID) {
    if (isEmbeddedMode()) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, msTimestamp * 1000, pipelineID);
    });
  }

  @Override
  public void markTiming(final String key, final String pipelineID) {
    long usTimestamp = currentSystemTimeMicroseconds();
    makeTraceEventInstant(MARK_TIMING, key, usTimestamp, pipelineID);
    if (isEmbeddedMode()) {
      ensureEmbeddedCollectorInitialized();
      mEmbeddedTimingCollector.markTiming(key, usTimestamp);
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, usTimestamp, pipelineID);
    });
  }

  @Override
  @UiThread
  public void markHostPlatformTiming(final String key) {
    if (isEmbeddedMode() || !UIThreadUtils.isOnUiThread() || mPendingPaintEndPipelineIds.isEmpty()
        || key == null) {
      return;
    }
    if (mHostPlatformTiming != null) {
      // To ensure timing accuracy, we only record the first 'start' event and overwrite 'end'
      // events, as measure events can be triggered multiple times within a single draw cycle.
      if (key.endsWith("Start") && mHostPlatformTiming.containsKey(key)) {
        return;
      }
    } else {
      mHostPlatformTiming = new JavaOnlyMap();
    }
    long usTimestamp = currentSystemTimeMicroseconds();
    makeTraceEventInstants(MARK_HOST_PLATFORM_TIMING, key, usTimestamp);
    if (mHostPlatformTiming == null) {
    }
    mHostPlatformTiming.put(key, usTimestamp);
  }

  @Override
  @UiThread
  public void markPaintEndTimingIfNeeded() {
    if (isEmbeddedMode()) {
      ensureEmbeddedCollectorInitialized();
      if (mEmbeddedTimingCollector.hasEmitTimingEvent()) {
        return;
      }
      long usTimestamp = currentSystemTimeMicroseconds();
      makeTraceEventInstant(MARK_TIMING, TIMING_KEY_PAINT_END, usTimestamp, "");
      mEmbeddedTimingCollector.markTiming(TIMING_KEY_PAINT_END, usTimestamp);
      return;
    }
    if (!UIThreadUtils.isOnUiThread() || mPendingPaintEndPipelineIds.isEmpty()) {
      return;
    }
    long usTimestamp = currentSystemTimeMicroseconds();
    makeTraceEventInstants(MARK_TIMING, TIMING_KEY_PAINT_END, usTimestamp);
    JavaOnlyMap hostPlatformTiming = mHostPlatformTiming;
    mHostPlatformTiming = null;
    JavaOnlyArray pendingPaintEndPipelineIds = mPendingPaintEndPipelineIds;
    mPendingPaintEndPipelineIds = new JavaOnlyArray();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetPaintEndTimingAndHostPlatformTiming(
          mNativePerformanceActorPtr, usTimestamp, hostPlatformTiming, pendingPaintEndPipelineIds);
    });
  }

  @Override
  @UiThread
  public void setNeedMarkPaintEndTiming(String pipelineId) {
    if (isEmbeddedMode() || !UIThreadUtils.isOnUiThread()) {
      // Embedded mode: no PaintEnd timing needed after fcp
      return;
    }
    mPendingPaintEndPipelineIds.add(pipelineId);
  }

  public void setFSPTimingInfo(long usTimestamp, Map<String, String> detail) {
    // TODO(limeng.amer): implemant by jni.
    if (isEmbeddedMode()) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetFSPTimingInfo(mNativePerformanceActorPtr, usTimestamp, detail);
    });
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (isEmbeddedMode()) {
      // Embedded mode: no extra timing needed
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      if (extraTiming.mOpenTime > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.OPEN_TIME,
            extraTiming.mOpenTime * 1000, null);
      }
      if (extraTiming.mContainerInitStart > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.CONTAINER_INIT_START,
            extraTiming.mContainerInitStart * 1000, null);
      }
      if (extraTiming.mContainerInitEnd > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.CONTAINER_INIT_END,
            extraTiming.mContainerInitEnd * 1000, null);
      }
      if (extraTiming.mPrepareTemplateStart > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.PREPARE_TEMPLATE_START,
            extraTiming.mPrepareTemplateStart * 1000, null);
      }
      if (extraTiming.mPrepareTemplateEnd > 0) {
        nativeSetTiming(mNativePerformanceActorPtr, TimingHandler.PREPARE_TEMPLATE_END,
            extraTiming.mPrepareTemplateEnd * 1000, null);
      }
    });
  }

  @CalledByNative
  protected void setNativePtr(long nativePtr) {
    if (isEmbeddedMode()) {
      return;
    }
    mNativePerformanceActorPtr = nativePtr;
  }

  @CalledByNative
  protected void onPerformanceEvent(ReadableMap entryMap) {
    if (isEmbeddedMode()) {
      return;
    }
    IPerformanceObserver observer = mObserver.get();
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (observer != null) {
      observer.onPerformanceEvent(entry);
    }

    ILynxEventReporterService reporter = getEventReporterService();
    if (reporter != null) {
      int instanceId = entryMap.getInt("instanceId", -1);
      if (instanceId != -1) {
        HashMap<String, Object> newEntryMap = LynxEventReporter.getGenericInfo(instanceId);
        newEntryMap.putAll(entryMap.asHashMap());
        PerformanceEntry newEntry =
            PerformanceEntryConverter.makePerformanceEntry(JavaOnlyMap.from(newEntryMap));
        reporter.onPerformanceEvent(newEntry);
      }
    }
  }

  @AnyThread
  private void runOnReportThread(Runnable runnable) {
    if (isEmbeddedMode()) {
      return;
    }
    LynxEventReporter.runOnReportThread(runnable);
  }

  private static boolean isNativeLibraryLoaded() {
    if (!sIsNativeLibraryLoaded) {
      sIsNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    }
    return sIsNativeLibraryLoaded;
  }

  private void makeTraceEventInstants(String prefix, String timingKey, long timestamp) {
    if (TraceEvent.isTracingStarted()) {
      for (Object id : mPendingPaintEndPipelineIds) {
        makeTraceEventInstant(prefix, timingKey, timestamp, (String) id);
      }
    }
  }

  private void makeTraceEventInstant(
      String prefix, String timingKey, long timestamp, String pipelineId) {
    if (TraceEvent.isTracingStarted()) {
      Map<String, String> props = new HashMap<>();
      props.put(TIMING_KEY, timingKey);
      props.put(TIMING_TIMESTAMP, String.valueOf(timestamp));
      props.put(PIPELINE_ID, pipelineId);
      props.put(INSTANCE_ID, String.valueOf(mInstanceId));
      TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT, prefix + "." + timingKey, props);
    }
  }

  public static long currentSystemTimeMicroseconds() {
    if (isNativeLibraryLoaded()) {
      return nativeCurrentSystemTimeMicroseconds();
    }
    LLog.e(TAG, "Failed to call currentSystemTimeMicroseconds to obtain the timestamp.");
    return 0;
  }

  private ILynxEventReporterService getEventReporterService() {
    if (mEventReporterService != null) {
      return mEventReporterService.get();
    }
    ILynxEventReporterService reporter =
        LynxServiceCenter.inst().getService(ILynxEventReporterService.class);
    if (reporter != null) {
      mEventReporterService = new WeakReference<>(reporter);
    }
    return reporter;
  }

  @Override
  public JavaOnlyArray getPendingPaintEndPipelineIds() {
    return mPendingPaintEndPipelineIds;
  }

  // Native API
  private native void nativeAllocateMemory(
      long nativePtr, String category, long sizeBytes, String detailKey, String detailValue);
  private native void nativeDeallocateMemory(
      long nativePtr, String category, long sizeBytes, String detailKey, String detailValue);
  private native void nativeUpdateMemoryUsage(long nativePtr, String category, long sizeBytes,
      int instanceCount, Map<String, String> detail);
  private native void nativeSetTiming(
      long nativePtr, String key, long usTimestamp, String pipelineID);
  private native void nativeSetPaintEndTimingAndHostPlatformTiming(
      long nativePtr, long usTimestamp, JavaOnlyMap hostPlatformTiming, JavaOnlyArray pipelineIds);
  private static native boolean nativeIsMemoryMonitorEnabled();
  private native void nativeSetFSPTimingInfo(
      long nativePtr, long usTimestamp, Map<String, String> detail);

  private static native long nativeCurrentSystemTimeMicroseconds();
}
