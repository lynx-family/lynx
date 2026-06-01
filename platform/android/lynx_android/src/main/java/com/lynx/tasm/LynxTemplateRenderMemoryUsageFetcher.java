// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Fetches one TemplateRender's memory snapshot for the process-wide memory query collector.
 *
 * <p>The fetcher is intentionally outside {@link LynxTemplateRender}: TemplateRender only owns
 * lifecycle registration plus the narrow native bridge, while this class owns the memory-query
 * orchestration across native, UI, and report threads.
 */
final class LynxTemplateRenderMemoryUsageFetcher extends LynxMemoryUsageFetcher {
  private final WeakReference<LynxTemplateRender> mRenderRef;

  LynxTemplateRenderMemoryUsageFetcher(@Nullable LynxTemplateRender render) {
    mRenderRef = new WeakReference<>(render);
  }

  @Override
  void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
    TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_QUERY);
    LynxTemplateRender render = mRenderRef.get();
    if (render == null) {
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_QUERY);
      callback.accept(null);
      return;
    }

    LynxContext lynxContext = render.getLynxContext();
    int instanceId =
        lynxContext == null ? LynxEventReporter.INSTANCE_ID_UNKNOWN : lynxContext.getInstanceId();
    String url = lynxContext == null ? render.getTemplateUrl() : lynxContext.getTemplateUrl();
    // Keep the UI owner for this accepted query through the UI-thread handoff. If teardown starts
    // before the UI task runs, this snapshot should still sample the view tree that belonged to the
    // accepted TemplateRender query instead of silently degrading to an empty view snapshot.
    LynxUIOwner uiOwner = lynxContext == null ? null : lynxContext.getLynxUIOwner();
    InstanceMemoryUsageQuery query = new InstanceMemoryUsageQuery(instanceId, url, callback);

    TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_DISPATCH_NATIVE);
    boolean nativeQueryStarted = render.queryNativeMemoryUsageForGlobalCollectorAsync(query);
    if (!nativeQueryStarted) {
      // The render may be between teardown phases. Complete the native half with zeros on the
      // report thread so the per-instance query can still finish once the view snapshot returns.
      LynxEventReporter.runOnReportThread(() -> query.completeNativeSnapshot(0L, 0L, 0L, 0L, null));
    }
    TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_DISPATCH_NATIVE);

    queryViewMemoryUsageAsync(uiOwner, query);
    TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_QUERY);
  }

  private static void queryViewMemoryUsageAsync(
      @Nullable LynxUIOwner uiOwner, @NonNull InstanceMemoryUsageQuery query) {
    UIThreadUtils.runOnUiThread(() -> {
      TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_VIEW_SAMPLE);
      HashMap<String, MemoryRecord> viewDetail = new HashMap<>();
      long viewBytes = sampleViewMemoryUsage(uiOwner, viewDetail);
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_VIEW_SAMPLE);
      LynxEventReporter.runOnReportThread(() -> {
        TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_VIEW_COMPLETE);
        query.completeViewSnapshot(viewBytes, viewDetail);
        TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_VIEW_COMPLETE);
      });
    });
  }

  private static long sampleViewMemoryUsage(
      @Nullable LynxUIOwner uiOwner, @NonNull HashMap<String, MemoryRecord> viewDetail) {
    HashMap<String, MemoryRecord> records = uiOwner == null ? null : uiOwner.getMemoryUsage();
    if (records == null || records.isEmpty()) {
      return 0L;
    }

    long viewBytes = 0;
    // MemoryRecord is mutable and owned by the UI tree. Copy every record while still on the UI
    // thread, then publish the detached snapshot to the report-thread aggregation query.
    for (Map.Entry<String, MemoryRecord> entry : records.entrySet()) {
      MemoryRecord record = entry.getValue();
      if (record == null) {
        continue;
      }
      long recordBytes = record.mSizeBytes;
      viewBytes += recordBytes;
      String key = entry.getKey();
      String category = record.getCategory();
      String detailKey = key == null || key.isEmpty() ? category : key;
      if (detailKey != null && !detailKey.isEmpty()) {
        viewDetail.put(detailKey, record.copy());
      }
    }
    return viewBytes;
  }

  /**
   * Aggregates native and Android view memory snapshots for one TemplateRender instance.
   *
   * <p>Native and view sampling finish on different threads. Each completion is posted back to the
   * Lynx report thread before it reaches this context, so all mutable fields below are
   * report-thread confined. That keeps the fan-in path easy to reason about and avoids adding locks
   * to the common collection flow.
   */
  @Keep
  static final class InstanceMemoryUsageQuery {
    private final int mInstanceId;
    @Nullable private final String mUrl;
    private final long mQueryStartMs = LynxGlobalMemoryUsageCollector.nowMs();
    @Nullable private LynxConsumer<LynxInstanceMemoryUsage> mCallback;
    private boolean mNativeDone;
    private boolean mViewDone;
    private boolean mFinished;
    private long mElementBytes;
    private long mElementNodeCount;
    private long mMainThreadRuntimeBytes;
    private long mBackgroundThreadRuntimeBytes;
    @Nullable private String mBtsRuntimeGroupId;
    private long mViewBytes;
    private Map<String, MemoryRecord> mViewDetail = Collections.emptyMap();

    InstanceMemoryUsageQuery(int instanceId, @Nullable String url,
        @NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
      mInstanceId = instanceId;
      mUrl = url;
      mCallback = callback;
    }

    void completeNativeSnapshot(long elementBytes, long elementNodeCount,
        long mainThreadRuntimeBytes, long backgroundThreadRuntimeBytes,
        @Nullable String btsRuntimeGroupId) {
      mElementBytes = elementBytes;
      mElementNodeCount = elementNodeCount;
      mMainThreadRuntimeBytes = mainThreadRuntimeBytes;
      mBackgroundThreadRuntimeBytes = backgroundThreadRuntimeBytes;
      mBtsRuntimeGroupId = btsRuntimeGroupId;
      mNativeDone = true;
      finishIfReady();
    }

    void completeViewSnapshot(long viewBytes, @Nullable Map<String, MemoryRecord> viewDetail) {
      mViewBytes = viewBytes;
      // Records are already detached by sampleViewMemoryUsage() on the UI thread. Keep only a
      // private map snapshot while waiting for the native half of this per-instance query.
      mViewDetail = viewDetail == null || viewDetail.isEmpty() ? Collections.emptyMap()
                                                               : new HashMap<>(viewDetail);
      mViewDone = true;
      finishIfReady();
    }

    private void finishIfReady() {
      if (mFinished || !mNativeDone || !mViewDone) {
        return;
      }
      TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_FINISH);
      mFinished = true;
      long totalBytes =
          mElementBytes + mViewBytes + mMainThreadRuntimeBytes + mBackgroundThreadRuntimeBytes;
      LynxInstanceMemoryUsage result = new LynxInstanceMemoryUsage(mInstanceId, null, mUrl,
          totalBytes, mElementBytes, mElementNodeCount, mViewBytes, mViewDetail,
          mMainThreadRuntimeBytes, mBackgroundThreadRuntimeBytes, mBtsRuntimeGroupId);

      if (TraceEvent.isTracingStarted()) {
        HashMap<String, String> traceProps = new HashMap<>();
        traceProps.put("instance_id", String.valueOf(mInstanceId));
        traceProps.put(
            "duration_ms", String.valueOf(LynxGlobalMemoryUsageCollector.nowMs() - mQueryStartMs));
        traceProps.put("total_bytes", String.valueOf(totalBytes));
        TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT,
            TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_TOTAL, traceProps);
      }

      LynxConsumer<LynxInstanceMemoryUsage> callback = mCallback;
      mCallback = null;
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_FINISH);
      if (callback != null) {
        callback.accept(result);
      }
    }

    /**
     * JNI callback target for native TemplateRender memory snapshots.
     *
     * <p>The callback can arrive from the native report path. It is immediately marshalled to the
     * Java report thread before touching query state, keeping native and view completion serialized
     * with the global collector timeout.
     */
    @CalledByNative
    private void onNativeMemoryUsageResult(long elementBytes, long elementNodeCount,
        long mainThreadRuntimeBytes, long backgroundThreadRuntimeBytes, String btsRuntimeGroupId) {
      TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_CALLBACK);
      LynxEventReporter.runOnReportThread(() -> {
        TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_COMPLETE);
        completeNativeSnapshot(elementBytes, elementNodeCount, mainThreadRuntimeBytes,
            backgroundThreadRuntimeBytes, btsRuntimeGroupId);
        TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_COMPLETE);
      });
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_CALLBACK);
    }
  }
}
