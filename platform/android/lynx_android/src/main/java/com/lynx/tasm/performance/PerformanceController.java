// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.IPerformanceObserver;
import com.lynx.tasm.performance.memory.IMemoryMonitor;
import com.lynx.tasm.performance.memory.IMemoryRecordBuilder;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.performance.timing.ITimingCollector;
import com.lynx.tasm.performance.timing.TimingUtil;
import java.lang.ref.WeakReference;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class PerformanceController implements IMemoryMonitor, ITimingCollector {
  private volatile long mNativePerformanceActorPtr = 0;
  private volatile boolean mIsNativeLibraryLoaded = false;
  private WeakReference<IPerformanceObserver> mObserver;

  public void setPerformanceObserver(IPerformanceObserver observer) {
    mObserver = new WeakReference<>(observer);
  }

  @Override
  public void allocateMemory(IMemoryRecordBuilder builder) {
    if (mNativePerformanceActorPtr == 0 || builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeAllocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void deallocateMemory(IMemoryRecordBuilder builder) {
    if (mNativePerformanceActorPtr == 0 || builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeDeallocateMemory(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void updateMemoryUsage(IMemoryRecordBuilder builder) {
    if (mNativePerformanceActorPtr == 0 || builder == null) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      MemoryRecord record = builder.build();
      nativeUpdateMemoryUsage(
          mNativePerformanceActorPtr, record.getCategory(), record.getSizeKb(), null, null);
    });
  }

  @Override
  public void setMsTiming(String key, long msTimestamp, String pipelineID) {
    if (mNativePerformanceActorPtr == 0) {
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
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetTiming(mNativePerformanceActorPtr, key, usTimestamp, pipelineID);
    });
  }

  @Override
  public void setNeedPaintEndTiming(String pipelineId) {
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeSetNeedPaintEndTiming(mNativePerformanceActorPtr, pipelineId);
    });
  }

  @Override
  public void markPaintEndTimingIfNeeded() {
    if (mNativePerformanceActorPtr == 0) {
      return;
    }
    long usTimestamp = TimingUtil.currentTimeUs();
    runOnReportThread(() -> {
      if (mNativePerformanceActorPtr == 0) {
        return;
      }
      nativeMarkPaintEndTimingIfNeeded(mNativePerformanceActorPtr, usTimestamp);
    });
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (extraTiming.mOpenTime > 0) {
      setMsTiming(TimingHandler.OPEN_TIME, extraTiming.mOpenTime, null);
    }
    if (extraTiming.mContainerInitStart > 0) {
      setMsTiming(TimingHandler.CONTAINER_INIT_START, extraTiming.mContainerInitStart, null);
    }
    if (extraTiming.mContainerInitEnd > 0) {
      setMsTiming(TimingHandler.CONTAINER_INIT_END, extraTiming.mContainerInitEnd, null);
    }
    if (extraTiming.mPrepareTemplateStart > 0) {
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_START, extraTiming.mPrepareTemplateStart, null);
    }
    if (extraTiming.mPrepareTemplateEnd > 0) {
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, extraTiming.mPrepareTemplateEnd, null);
    }
  }

  @CalledByNative
  protected void setNativePtr(long nativePtr) {
    mNativePerformanceActorPtr = nativePtr;
  }

  @CalledByNative
  protected void onPerformanceEvent(ReadableMap entryMap) {
    IPerformanceObserver observer = mObserver.get();
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(entryMap);
    if (observer != null) {
      observer.onPerformanceEvent(entry);
    }
  }

  @AnyThread
  private void runOnReportThread(Runnable runnable) {
    LynxEventReporter.runOnReportThread(runnable);
  }

  private native void nativeAllocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);

  private native void nativeDeallocateMemory(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);

  private native void nativeUpdateMemoryUsage(
      long nativePtr, String category, float sizeKb, String detailKey, String detailValue);
  private native void nativeSetForceEnable(long nativePtr, boolean forceEnable);
  private native void nativeSetTiming(
      long nativePtr, String key, long usTimestamp, String pipelineID);
  private native void nativeSetNeedPaintEndTiming(long nativePtr, String pipelineID);
  private native void nativeMarkPaintEndTimingIfNeeded(long nativePtr, long usTimestamp);
}
