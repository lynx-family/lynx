// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.os.Debug;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Coordinates one global memory query across all registered Lynx instance fetchers.
 *
 * <p>The collector owns the global query lifecycle: it snapshots live fetchers, coalesces
 * concurrent public API calls into a single in-flight query, enforces the global timeout, and
 * builds the final aggregate result. Mutable query state is confined to the Lynx report thread
 * whenever the native library is loaded.
 */
class LynxGlobalMemoryUsageCollector {
  private static final String TAG = "LynxMemoryCollector";
  private static final long GLOBAL_MEMORY_USAGE_TIMEOUT_MS = 2000;
  private static final LynxGlobalMemoryUsageCollector INSTANCE =
      new LynxGlobalMemoryUsageCollector();

  // Fetchers are owned by live Lynx instances. Teardown still unregisters them explicitly, but the
  // registry is weak so a missed unregister does not keep an instance alive. The concurrent set
  // avoids a global registry monitor for apps that host many Lynx instances.
  private final Set<WeakFetcherRef> mFetcherRefs = ConcurrentHashMap.newKeySet();
  // Non-null only while a collector-backed global query is running on the report thread.
  @Nullable private LynxGlobalMemoryUsageCollectionContext mPendingContext;

  static LynxGlobalMemoryUsageCollector getInstance() {
    return INSTANCE;
  }

  @AnyThread
  void queryMemoryUsageAsync(@Nullable LynxGlobalMemoryUsageCallback callback) {
    if (callback == null) {
      return;
    }
    TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_GLOBAL_QUERY);
    long collectionStartMs = nowMs();
    boolean isNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    if (!isNativeLibraryLoaded) {
      // The report thread is created by native initialization. Before that point there can be no
      // native collector or registered instance fetchers, so return the public empty result without
      // touching report-thread-only state.
      runOnFallbackThread(() -> {
        LynxGlobalMemoryUsageCollectionContext.invokeCallbackSafely(callback,
            LynxGlobalMemoryUsageResult.build(collectionStartMs,
                LynxMemoryCollectionStatus.COMPLETED, nowMs() - collectionStartMs,
                GLOBAL_MEMORY_USAGE_TIMEOUT_MS, 0, sampleAppBytes(), Collections.emptyList()));
      });
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_QUERY);
      return;
    }

    runOnReportThread(() -> queryMemoryUsageOnReportThread(callback, collectionStartMs));
    TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_QUERY);
  }

  @AnyThread
  boolean registerMemoryUsageFetcher(@NonNull LynxMemoryUsageFetcher fetcher) {
    return mFetcherRefs.add(new WeakFetcherRef(fetcher));
  }

  @AnyThread
  boolean unregisterMemoryUsageFetcher(@NonNull LynxMemoryUsageFetcher fetcher) {
    return mFetcherRefs.remove(new WeakFetcherRef(fetcher));
  }

  private void queryMemoryUsageOnReportThread(
      @NonNull LynxGlobalMemoryUsageCallback callback, long collectionStartMs) {
    TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_GLOBAL_REPORT_THREAD_QUERY);
    LynxGlobalMemoryUsageCollectionContext pendingContext = mPendingContext;
    if (pendingContext != null) {
      // A query is already collecting. Keep the expensive per-instance fetch work shared, and fan
      // out the same final result to every coalesced API callback.
      pendingContext.addCallback(callback);
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_REPORT_THREAD_QUERY);
      return;
    }

    // Snapshot the registry once so the expected count remains stable for this query, even if
    // instances are created or destroyed while collection is in flight.
    List<LynxMemoryUsageFetcher> fetchers = fetchersForCurrentQuery();

    LynxGlobalMemoryUsageCollectionContext context = new LynxGlobalMemoryUsageCollectionContext(
        collectionStartMs, GLOBAL_MEMORY_USAGE_TIMEOUT_MS, fetchers.size(), callback);
    context.setFinishHandler(finishedContext -> {
      if (mPendingContext == finishedContext) {
        mPendingContext = null;
      }
    });
    mPendingContext = context;

    if (fetchers.isEmpty()) {
      // The public API has a real collector now, but there may be no live Lynx instances.
      // Complete asynchronously with an empty successful result instead of waiting for the timeout.
      context.finishWithStatus(LynxMemoryCollectionStatus.COMPLETED);
      TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_REPORT_THREAD_QUERY);
      return;
    }

    delayRunOnReportThread(()
                               -> context.finishWithStatus(LynxMemoryCollectionStatus.TIMEOUT),
        GLOBAL_MEMORY_USAGE_TIMEOUT_MS);

    TraceEvent.beginSection(TraceEventDef.MEMORY_USAGE_GLOBAL_FETCHER_FAN_OUT);
    for (LynxMemoryUsageFetcher fetcher : fetchers) {
      LynxConsumer<LynxInstanceMemoryUsage> singleShotCallback =
          createSingleShotCallback(result -> didReceiveFetcherResult(context, result));
      try {
        fetcher.queryMemoryUsageAsync(singleShotCallback);
      } catch (Throwable throwable) {
        // A fetcher bug should count as one failed instance fetch, not stall the whole global
        // query. Log it because a silent fallback makes integration bugs hard to diagnose.
        LLog.e(TAG, "Failed to query Lynx memory usage fetcher: " + throwable.getMessage());
        singleShotCallback.accept(null);
      }
    }
    TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_FETCHER_FAN_OUT);
    TraceEvent.endSection(TraceEventDef.MEMORY_USAGE_GLOBAL_REPORT_THREAD_QUERY);
  }

  private void didReceiveFetcherResult(@NonNull LynxGlobalMemoryUsageCollectionContext context,
      @Nullable LynxInstanceMemoryUsage result) {
    // Fetchers may finish on UI, native, or background threads. Mutate the shared context only from
    // the report thread so timeout and result completion are serialized.
    runOnReportThread(() -> { context.didReceiveInstanceResult(result); });
  }

  /**
   * Protects the global query from a misbehaving fetcher invoking its callback more than once.
   *
   * <p>{@link LynxConsumer} is Lynx's min-SDK-compatible equivalent of
   * {@code java.util.function.Consumer}. In this collector it represents the per-fetcher result
   * callback: one fetcher should call {@link LynxConsumer#accept(Object)} once with its memory
   * snapshot, or with {@code null} when it cannot produce one.
   */
  @NonNull
  static LynxConsumer<LynxInstanceMemoryUsage> createSingleShotCallback(
      @NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
    AtomicBoolean didCall = new AtomicBoolean(false);
    return result -> {
      if (didCall.compareAndSet(false, true)) {
        try {
          callback.accept(result);
        } catch (Throwable throwable) {
          // The single-shot wrapper is the fetcher boundary. Keep callback exceptions from
          // escaping to UI/native/background fetcher threads.
          LLog.e(
              TAG, "Failed to handle Lynx memory usage fetcher result: " + throwable.getMessage());
        }
      }
    };
  }

  @AnyThread
  private static void runOnFallbackThread(@NonNull Runnable runnable) {
    // The native report thread is created with the native library. Before that,
    // keep the API asynchronous instead of dropping the task through
    // LynxEventReporter.
    LynxThreadPool.getAsyncServiceExecutor().execute(runnable);
  }

  void runOnReportThread(@NonNull Runnable runnable) {
    LynxEventReporter.runOnReportThread(runnable);
  }

  void delayRunOnReportThread(@NonNull Runnable runnable, long delayMs) {
    LynxEventReporter.delayRunOnReportThread(runnable, delayMs);
  }

  @NonNull
  private List<LynxMemoryUsageFetcher> fetchersForCurrentQuery() {
    // A global query must ask every live instance once, so one O(n) registry snapshot is
    // unavoidable. While walking, drop cleared weak refs and make the expected instance count
    // stable until all fetchers have either replied or timed out.
    List<LynxMemoryUsageFetcher> fetchers = new ArrayList<>();
    for (WeakFetcherRef fetcherRef : mFetcherRefs) {
      LynxMemoryUsageFetcher fetcher = fetcherRef.get();
      if (fetcher == null) {
        mFetcherRefs.remove(fetcherRef);
        continue;
      }
      fetchers.add(fetcher);
    }
    return fetchers;
  }

  static long nowMs() {
    return System.currentTimeMillis();
  }

  static long sampleAppBytes() {
    try {
      Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
      Debug.getMemoryInfo(memoryInfo);
      // Android reports PSS in KB. Public API fields use bytes.
      return Math.max(0L, memoryInfo.getTotalPss()) * 1024L;
    } catch (Throwable throwable) {
      LLog.e(TAG, "Failed to sample app memory usage: " + throwable.getMessage());
      return 0;
    }
  }

  private static final class WeakFetcherRef extends WeakReference<LynxMemoryUsageFetcher> {
    private final int mHashCode;

    WeakFetcherRef(@NonNull LynxMemoryUsageFetcher fetcher) {
      super(fetcher);
      mHashCode = System.identityHashCode(fetcher);
    }

    @Override
    public int hashCode() {
      return mHashCode;
    }

    @Override
    public boolean equals(@Nullable Object obj) {
      if (this == obj) {
        return true;
      }
      if (!(obj instanceof WeakFetcherRef)) {
        return false;
      }
      LynxMemoryUsageFetcher fetcher = get();
      LynxMemoryUsageFetcher otherFetcher = ((WeakFetcherRef) obj).get();
      return fetcher != null && fetcher == otherFetcher;
    }
  }
}
