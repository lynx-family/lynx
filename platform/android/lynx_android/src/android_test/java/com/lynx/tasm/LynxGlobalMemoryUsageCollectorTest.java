// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.LynxMemoryUsageTestUtils.createInstance;
import static com.lynx.tasm.LynxMemoryUsageTestUtils.createNoOpFetcher;
import static com.lynx.tasm.LynxMemoryUsageTestUtils.createResultCallback;
import static com.lynx.tasm.LynxMemoryUsageTestUtils.runWithNativeLibraryLoaded;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxMemoryUsageTestUtils.TestCollector;
import com.lynx.tasm.base.LynxConsumer;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxGlobalMemoryUsageCollectorTest {
  @Test
  public void returnsEmptyCompletedResultWhenNativeLibraryIsUnavailable() throws Exception {
    LynxEnv env = LynxEnv.inst();
    boolean wasNativeLibraryLoaded = env.mIsNativeLibraryLoaded;
    env.setNativeLibraryLoaded(false);
    CountDownLatch latch = new CountDownLatch(1);
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();

    try {
      new LynxGlobalMemoryUsageCollector().queryMemoryUsageAsync(
          new LynxGlobalMemoryUsageCallback() {
            @Override
            public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
              resultRef.set(result);
              latch.countDown();
            }
          });

      assertTrue(latch.await(1, TimeUnit.SECONDS));
    } finally {
      env.setNativeLibraryLoaded(wasNativeLibraryLoaded);
    }

    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertTrue(result.getInstances().isEmpty());
  }

  @Test
  public void doesNotStartFetchersWhenNativeLibraryIsUnavailable() throws Exception {
    LynxEnv env = LynxEnv.inst();
    boolean wasNativeLibraryLoaded = env.mIsNativeLibraryLoaded;
    env.setNativeLibraryLoaded(false);
    CountDownLatch latch = new CountDownLatch(1);
    AtomicInteger fetcherCallCount = new AtomicInteger();
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
    LynxMemoryUsageFetcher fetcher = new LynxMemoryUsageFetcher() {
      @Override
      void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
        fetcherCallCount.incrementAndGet();
        callback.accept(createInstance(1, 1L, 1L, 1L, 1L, 1L, 1L, null));
      }
    };
    LynxGlobalMemoryUsageCollector collector = new LynxGlobalMemoryUsageCollector();
    assertTrue(collector.registerMemoryUsageFetcher(fetcher));

    try {
      collector.queryMemoryUsageAsync(new LynxGlobalMemoryUsageCallback() {
        @Override
        public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
          resultRef.set(result);
          latch.countDown();
        }
      });

      assertTrue(latch.await(1, TimeUnit.SECONDS));
    } finally {
      env.setNativeLibraryLoaded(wasNativeLibraryLoaded);
    }

    assertEquals(0, fetcherCallCount.get());
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
  }

  @Test
  public void completesOnReportThreadWhenNoFetchers() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();

      collector.queryMemoryUsageAsync(createResultCallback(resultRef));

      assertNull(resultRef.get());
      assertEquals(1, collector.getReportTaskCount());
      collector.runNextReportTask();

      LynxGlobalMemoryUsageResult result = resultRef.get();
      assertNotNull(result);
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
      assertEquals(0, result.getExpectedInstanceCount());
      assertEquals(0, result.getCompletedInstanceCount());
      assertTrue(result.getInstances().isEmpty());
    });
  }

  @Test
  public void runsRegisteredFetcherThroughReportThreadAndCompletes() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicInteger fetcherCallCount = new AtomicInteger();
      AtomicInteger callbackCount = new AtomicInteger();
      AtomicReference<LynxConsumer<LynxInstanceMemoryUsage>> fetcherCallback =
          new AtomicReference<>();
      AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher fetcher = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          fetcherCallCount.incrementAndGet();
          fetcherCallback.set(callback);
        }
      };
      assertTrue(collector.registerMemoryUsageFetcher(fetcher));

      collector.queryMemoryUsageAsync(new LynxGlobalMemoryUsageCallback() {
        @Override
        public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
          callbackCount.incrementAndGet();
          resultRef.set(result);
        }
      });

      assertNull(fetcherCallback.get());
      assertEquals(1, collector.getReportTaskCount());
      collector.runNextReportTask();
      assertEquals(1, fetcherCallCount.get());
      assertNotNull(fetcherCallback.get());
      assertEquals(1, collector.getDelayedTaskCount());
      assertEquals(2000L, collector.getDelayedMs(0));

      fetcherCallback.get().accept(createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, "shared"));
      assertNull(resultRef.get());
      assertEquals(1, collector.getReportTaskCount());
      collector.runNextReportTask();

      LynxGlobalMemoryUsageResult result = resultRef.get();
      assertNotNull(result);
      assertEquals(1, callbackCount.get());
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
      assertEquals(1, result.getExpectedInstanceCount());
      assertEquals(1, result.getCompletedInstanceCount());
      assertEquals(187L, result.getTotalBytes());
      assertTrue(result.getCollectionDurationMs() >= 0L);
      assertTrue(result.getAppBytes() >= 0L);

      collector.runNextDelayedTask();
      assertEquals(1, callbackCount.get());
    });
  }

  @Test
  public void timesOutRegisteredFetcherThroughReportThread() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher fetcher = createNoOpFetcher();
      assertTrue(collector.registerMemoryUsageFetcher(fetcher));

      collector.queryMemoryUsageAsync(createResultCallback(resultRef));
      collector.runNextReportTask();
      assertNull(resultRef.get());
      assertEquals(1, collector.getDelayedTaskCount());

      collector.runNextDelayedTask();

      LynxGlobalMemoryUsageResult result = resultRef.get();
      assertNotNull(result);
      assertEquals(LynxMemoryCollectionStatus.TIMEOUT, result.getCollectionStatus());
      assertTrue(result.getCollectionDurationMs() >= 0L);
      assertEquals(1, result.getExpectedInstanceCount());
      assertEquals(0, result.getCompletedInstanceCount());
      assertTrue(result.getInstances().isEmpty());
    });
  }

  @Test
  public void coalescesConcurrentQueriesIntoOneFetcherRequest() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicInteger fetcherCallCount = new AtomicInteger();
      AtomicReference<LynxConsumer<LynxInstanceMemoryUsage>> fetcherCallback =
          new AtomicReference<>();
      AtomicReference<LynxGlobalMemoryUsageResult> firstResultRef = new AtomicReference<>();
      AtomicReference<LynxGlobalMemoryUsageResult> secondResultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher fetcher = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          fetcherCallCount.incrementAndGet();
          fetcherCallback.set(callback);
        }
      };
      assertTrue(collector.registerMemoryUsageFetcher(fetcher));

      collector.queryMemoryUsageAsync(createResultCallback(firstResultRef));
      collector.runNextReportTask();
      collector.queryMemoryUsageAsync(createResultCallback(secondResultRef));
      collector.runNextReportTask();
      assertEquals(1, fetcherCallCount.get());

      fetcherCallback.get().accept(createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, null));
      collector.runNextReportTask();

      LynxGlobalMemoryUsageResult firstResult = firstResultRef.get();
      LynxGlobalMemoryUsageResult secondResult = secondResultRef.get();
      assertNotNull(firstResult);
      assertSame(firstResult, secondResult);
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, firstResult.getCollectionStatus());
      assertEquals(1, firstResult.getExpectedInstanceCount());
      assertEquals(1, firstResult.getCompletedInstanceCount());
    });
  }

  @Test
  public void ignoresFetcherThrowAfterCallback() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher fetcher = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          callback.accept(createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, null));
          throw new RuntimeException("throw after callback");
        }
      };
      assertTrue(collector.registerMemoryUsageFetcher(fetcher));

      collector.queryMemoryUsageAsync(createResultCallback(resultRef));
      collector.runNextReportTask();
      collector.runNextReportTask();

      LynxGlobalMemoryUsageResult result = resultRef.get();
      assertNotNull(result);
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
      assertEquals(1, result.getExpectedInstanceCount());
      assertEquals(1, result.getCompletedInstanceCount());
      assertEquals(187L, result.getTotalBytes());
    });
  }

  @Test
  public void completesWhenAllFetchersReturnNull() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher first = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          callback.accept(null);
        }
      };
      LynxMemoryUsageFetcher second = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          callback.accept(null);
        }
      };
      assertTrue(collector.registerMemoryUsageFetcher(first));
      assertTrue(collector.registerMemoryUsageFetcher(second));

      collector.queryMemoryUsageAsync(createResultCallback(resultRef));
      collector.runNextReportTask();
      collector.runNextReportTask();
      assertNull(resultRef.get());
      collector.runNextReportTask();

      LynxGlobalMemoryUsageResult result = resultRef.get();
      assertNotNull(result);
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
      assertEquals(2, result.getExpectedInstanceCount());
      assertEquals(0, result.getCompletedInstanceCount());
      assertEquals(0L, result.getTotalBytes());
      assertTrue(result.getInstances().isEmpty());
    });
  }

  @Test
  public void allowsUnregisteredInFlightFetcherToReturnNull() throws Exception {
    runWithNativeLibraryLoaded(() -> {
      TestCollector collector = new TestCollector();
      AtomicReference<LynxConsumer<LynxInstanceMemoryUsage>> fetcherCallback =
          new AtomicReference<>();
      AtomicReference<LynxGlobalMemoryUsageResult> firstResultRef = new AtomicReference<>();
      AtomicReference<LynxGlobalMemoryUsageResult> secondResultRef = new AtomicReference<>();
      LynxMemoryUsageFetcher fetcher = new LynxMemoryUsageFetcher() {
        @Override
        void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {
          fetcherCallback.set(callback);
        }
      };
      assertTrue(collector.registerMemoryUsageFetcher(fetcher));

      collector.queryMemoryUsageAsync(createResultCallback(firstResultRef));
      collector.runNextReportTask();
      assertTrue(collector.unregisterMemoryUsageFetcher(fetcher));

      fetcherCallback.get().accept(null);
      collector.runNextReportTask();
      LynxGlobalMemoryUsageResult firstResult = firstResultRef.get();
      assertNotNull(firstResult);
      assertEquals(LynxMemoryCollectionStatus.COMPLETED, firstResult.getCollectionStatus());
      assertEquals(1, firstResult.getExpectedInstanceCount());
      assertEquals(0, firstResult.getCompletedInstanceCount());

      collector.queryMemoryUsageAsync(createResultCallback(secondResultRef));
      collector.runNextReportTask();
      LynxGlobalMemoryUsageResult secondResult = secondResultRef.get();
      assertNotNull(secondResult);
      assertEquals(0, secondResult.getExpectedInstanceCount());
      assertEquals(0, secondResult.getCompletedInstanceCount());
    });
  }

  @Test
  public void singleShotCallbackIgnoresDuplicateFetcherResults() {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicReference<LynxInstanceMemoryUsage> resultRef = new AtomicReference<>();
    LynxInstanceMemoryUsage first = createInstance(1, 10L, 1L, 1L, 1L, 1L, 1L, "shared");
    LynxInstanceMemoryUsage second = createInstance(2, 20L, 2L, 2L, 2L, 2L, 2L, "shared");

    LynxConsumer<LynxInstanceMemoryUsage> singleShotCallback =
        LynxGlobalMemoryUsageCollector.createSingleShotCallback(result -> {
          callbackCount.incrementAndGet();
          resultRef.set(result);
        });

    singleShotCallback.accept(first);
    singleShotCallback.accept(second);
    singleShotCallback.accept(null);

    assertEquals(1, callbackCount.get());
    assertEquals(first, resultRef.get());
  }

  @Test
  public void singleShotCallbackDoesNotThrowFetcherCallbackExceptions() {
    AtomicInteger callbackCount = new AtomicInteger();
    LynxConsumer<LynxInstanceMemoryUsage> singleShotCallback =
        LynxGlobalMemoryUsageCollector.createSingleShotCallback(result -> {
          callbackCount.incrementAndGet();
          throw new RuntimeException("test callback exception");
        });

    singleShotCallback.accept(createInstance(1, 10L, 1L, 1L, 1L, 1L, 1L, "shared"));
    singleShotCallback.accept(createInstance(2, 20L, 2L, 2L, 2L, 2L, 2L, "shared"));

    assertEquals(1, callbackCount.get());
  }

  @Test
  public void keepsUniqueLiveFetchers() {
    LynxGlobalMemoryUsageCollector collector = new LynxGlobalMemoryUsageCollector();
    LynxMemoryUsageFetcher first = createNoOpFetcher();
    LynxMemoryUsageFetcher second = createNoOpFetcher();

    assertTrue(collector.registerMemoryUsageFetcher(first));
    assertFalse(collector.registerMemoryUsageFetcher(first));
    assertTrue(collector.registerMemoryUsageFetcher(second));
    assertTrue(collector.unregisterMemoryUsageFetcher(first));
    assertFalse(collector.unregisterMemoryUsageFetcher(first));
  }

  @Test
  public void snapshotsFetchersForCurrentQuery() throws Exception {
    LynxGlobalMemoryUsageCollector collector = new LynxGlobalMemoryUsageCollector();
    LynxMemoryUsageFetcher first = createNoOpFetcher();
    LynxMemoryUsageFetcher second = createNoOpFetcher();

    assertTrue(collector.registerMemoryUsageFetcher(first));
    assertTrue(collector.registerMemoryUsageFetcher(second));

    List<LynxMemoryUsageFetcher> snapshot = fetchersForCurrentQuery(collector);
    assertTrue(snapshot.contains(first));
    assertTrue(snapshot.contains(second));
    assertEquals(2, snapshot.size());

    assertTrue(collector.unregisterMemoryUsageFetcher(first));
    assertTrue(snapshot.contains(first));
    List<LynxMemoryUsageFetcher> nextSnapshot = fetchersForCurrentQuery(collector);
    assertFalse(nextSnapshot.contains(first));
    assertTrue(nextSnapshot.contains(second));
    assertEquals(1, nextSnapshot.size());
  }

  @Test
  public void dropsClearedWeakFetcherRefsFromSnapshots() throws Exception {
    LynxGlobalMemoryUsageCollector collector = new LynxGlobalMemoryUsageCollector();
    LynxMemoryUsageFetcher fetcher = createNoOpFetcher();

    assertTrue(collector.registerMemoryUsageFetcher(fetcher));
    List<LynxMemoryUsageFetcher> snapshot = fetchersForCurrentQuery(collector);
    assertEquals(1, snapshot.size());
    assertSame(fetcher, snapshot.get(0));

    clearRegisteredFetcherRefs(collector);

    assertTrue(fetchersForCurrentQuery(collector).isEmpty());
    assertFalse(collector.unregisterMemoryUsageFetcher(fetcher));
  }

  @SuppressWarnings("unchecked")
  private static List<LynxMemoryUsageFetcher> fetchersForCurrentQuery(
      LynxGlobalMemoryUsageCollector collector) throws Exception {
    Method fetchersForCurrentQuery =
        LynxGlobalMemoryUsageCollector.class.getDeclaredMethod("fetchersForCurrentQuery");
    fetchersForCurrentQuery.setAccessible(true);
    return (List<LynxMemoryUsageFetcher>) fetchersForCurrentQuery.invoke(collector);
  }

  private static void clearRegisteredFetcherRefs(LynxGlobalMemoryUsageCollector collector)
      throws Exception {
    Field fetcherRefsField = LynxGlobalMemoryUsageCollector.class.getDeclaredField("mFetcherRefs");
    fetcherRefsField.setAccessible(true);
    Set<?> fetcherRefs = (Set<?>) fetcherRefsField.get(collector);
    for (Object fetcherRef : fetcherRefs) {
      ((WeakReference<?>) fetcherRef).clear();
    }
  }
}
