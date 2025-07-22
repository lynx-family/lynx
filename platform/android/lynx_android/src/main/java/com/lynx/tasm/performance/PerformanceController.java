// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static com.lynx.tasm.base.trace.TraceEventDef.INSTANCE_ID;
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
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.memory.IMemoryMonitor;
import com.lynx.tasm.performance.memory.IMemoryRecordBuilder;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.performance.timing.ITimingCollector;
import com.lynx.tasm.performance.timing.TimingUtil;
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
  private static volatile boolean sIsNativeLibraryLoaded = false;
  private static volatile LynxBooleanOption sIsMemoryMonitorEnabled = LynxBooleanOption.UNSET;
  private static volatile long sMemoryAcquisitionDelaySec = -1;
  private volatile long mNativePerformanceActorPtr = 0;
  private WeakReference<IPerformanceObserver> mObserver;
  private WeakReference<ILynxEventReporterService> mEventReporterService;
  private boolean mEnableController = true;
  private JavaOnlyArray mPendingPaintEndPipelineIds = new JavaOnlyArray();
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;

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
  }

  public boolean isEnableController() {
    return mEnableController;
  }

  /**
   * Call this interface to disable performance monitoring only in Embedded Mode.
   */
  public void setEnableController(boolean enableController) {
    mEnableController = enableController;
  }

  public void setInstanceId(int instanceId) {
    mInstanceId = instanceId;
  }

  @Override
  public void allocateMemory(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
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
          mNativePerformanceActorPtr, record.getCategory(), record.mSizeKb, null, null);
    });
  }

  @Override
  public void deallocateMemory(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
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
          mNativePerformanceActorPtr, record.getCategory(), record.mSizeKb, null, null);
    });
  }

  @Override
  public void updateMemoryUsage(IMemoryRecordBuilder builder) {
    if (!mEnableController) {
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
      nativeUpdateMemoryUsage(mNativePerformanceActorPtr, record.getCategory(), record.mSizeKb,
          record.mInstanceCount, null);
    });
  }

  @Override
  public void updateMemoryUsage(Map<String, MemoryRecord> recordMap) {
    if (!mEnableController || recordMap == null) {
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
        nativeUpdateMemoryUsage(mNativePerformanceActorPtr, record.getCategory(), record.mSizeKb,
            record.mInstanceCount, record.mDetail);
      }
    });
  }

  @Override
  public void setMsTiming(String key, long msTimestamp, String pipelineID) {
    if (!mEnableController) {
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
    if (!mEnableController) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    makeTraceEventInstant(MARK_TIMING, key, usTimestamp, pipelineID);
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, usTimestamp, pipelineID);
    });
  }
  @Override
  @UiThread
  public void markPaintEndTimingIfNeeded() {
    if (!mEnableController || !UIThreadUtils.isOnUiThread()
        || mPendingPaintEndPipelineIds.isEmpty()) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    makeTraceEventInstants(MARK_TIMING, TIMING_KEY_PAINT_END, usTimestamp);
    JavaOnlyArray pendingPaintEndPipelineIds = mPendingPaintEndPipelineIds;
    mPendingPaintEndPipelineIds = new JavaOnlyArray();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetPaintEndTiming(mNativePerformanceActorPtr, usTimestamp, pendingPaintEndPipelineIds);
    });
  }

  @Override
  @UiThread
  public void setNeedMarkPaintEndTiming(String pipelineId) {
    if (!mEnableController || !UIThreadUtils.isOnUiThread()) {
      return;
    }
    mPendingPaintEndPipelineIds.add(pipelineId);
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (!mEnableController) {
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

  public void clearExtraTiming() {
    if (!mEnableController) {
      return;
    }
    nativeClearExtraTiming(mNativePerformanceActorPtr);
  }

  @CalledByNative
  protected void setNativePtr(long nativePtr) {
    if (!mEnableController) {
      return;
    }
    mNativePerformanceActorPtr = nativePtr;
  }

  @CalledByNative
  protected void onPerformanceEvent(ReadableMap entryMap) {
    if (!mEnableController) {
      return;
    }
    IPerformanceObserver observer = mObserver.get();
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (observer != null) {
      observer.onPerformanceEvent(entry);
    }

    if (mEventReporterService == null) {
      ILynxEventReporterService reporter =
          LynxServiceCenter.inst().getService(ILynxEventReporterService.class);
      if (reporter != null) {
        mEventReporterService = new WeakReference<>(reporter);
      }
    }
    ILynxEventReporterService reporter = mEventReporterService.get();
    if (reporter != null) {
      int instanceId = entryMap.getInt("instanceId");
      if (instanceId > 0) {
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
    if (!mEnableController) {
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

  // Native API
  private native void nativeAllocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeDeallocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeUpdateMemoryUsage(
      long nativePtr, String category, float sizeKb, int instanceCount, Map<String, String> detail);
  private native void nativeSetTiming(
      long nativePtr, String key, long usTimestamp, String pipelineID);
  private native void nativeSetPaintEndTiming(
      long nativePtr, long usTimestamp, JavaOnlyArray pipelineIds);
  private static native void nativeClearExtraTiming(long nativePtr);
  private static native boolean nativeIsMemoryMonitorEnabled();
}
