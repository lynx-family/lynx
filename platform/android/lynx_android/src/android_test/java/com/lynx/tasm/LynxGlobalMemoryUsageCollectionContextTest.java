// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.LynxMemoryUsageTestUtils.createInstance;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import androidx.annotation.NonNull;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxGlobalMemoryUsageCollectionContextTest {
  @Test
  public void finishesTimeoutWithPartialResultOnce() {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicInteger finishCount = new AtomicInteger();
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
    LynxInstanceMemoryUsage instance = createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, "shared");

    LynxGlobalMemoryUsageCollectionContext context = new LynxGlobalMemoryUsageCollectionContext(
        100L, 2000L, 2, new LynxGlobalMemoryUsageCallback() {
          @Override
          public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
            callbackCount.incrementAndGet();
            resultRef.set(result);
          }
        });
    context.addCallback(new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        callbackCount.incrementAndGet();
      }
    });
    context.setFinishHandler(finishedContext -> finishCount.incrementAndGet());
    context.didReceiveInstanceResult(instance);

    context.finishWithStatus(LynxMemoryCollectionStatus.TIMEOUT);
    context.didReceiveInstanceResult(createInstance(2, 1L, 1L, 1L, 1L, 1L, 1L, null));
    context.finishWithStatus(LynxMemoryCollectionStatus.COMPLETED);

    assertEquals(2, callbackCount.get());
    assertEquals(1, finishCount.get());
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.TIMEOUT, result.getCollectionStatus());
    assertEquals(2, result.getExpectedInstanceCount());
    assertEquals(1, result.getCompletedInstanceCount());
    assertEquals(187L, result.getTotalBytes());
  }

  @Test
  public void deliversRemainingCallbacksWhenOneCallbackThrows() {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();

    LynxGlobalMemoryUsageCollectionContext context = new LynxGlobalMemoryUsageCollectionContext(
        100L, 2000L, 0, new LynxGlobalMemoryUsageCallback() {
          @Override
          public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
            callbackCount.incrementAndGet();
            throw new RuntimeException("callback failure");
          }
        });
    context.addCallback(new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        callbackCount.incrementAndGet();
        resultRef.set(result);
      }
    });

    context.finishWithStatus(LynxMemoryCollectionStatus.COMPLETED);

    assertEquals(2, callbackCount.get());
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
  }

  @Test
  public void countsNullResultAndFinishesWhenExpectedCountReached() {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
    LynxInstanceMemoryUsage instance = createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, null);

    LynxGlobalMemoryUsageCollectionContext context = new LynxGlobalMemoryUsageCollectionContext(
        100L, 2000L, 2, new LynxGlobalMemoryUsageCallback() {
          @Override
          public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
            callbackCount.incrementAndGet();
            resultRef.set(result);
          }
        });

    context.didReceiveInstanceResult(null);
    assertEquals(1, context.getReceivedFetchResultCount());
    assertEquals(1, context.getCallbackCount());
    assertNull(resultRef.get());

    context.addCallback(new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        callbackCount.incrementAndGet();
      }
    });
    context.didReceiveInstanceResult(instance);
    context.addCallback(new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        callbackCount.incrementAndGet();
      }
    });

    assertEquals(2, callbackCount.get());
    assertEquals(2, context.getReceivedFetchResultCount());
    assertEquals(0, context.getCallbackCount());
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(2, result.getExpectedInstanceCount());
    assertEquals(1, result.getCompletedInstanceCount());
    assertEquals(187L, result.getTotalBytes());
  }

  @Test
  public void ignoresNullAndLateCallbacksAfterFinish() {
    AtomicInteger callbackCount = new AtomicInteger();
    AtomicInteger finishCount = new AtomicInteger();
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();

    LynxGlobalMemoryUsageCollectionContext context = new LynxGlobalMemoryUsageCollectionContext(
        100L, 2000L, 3, new LynxGlobalMemoryUsageCallback() {
          @Override
          public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
            callbackCount.incrementAndGet();
            resultRef.set(result);
          }
        });
    context.addCallback(null);
    context.setFinishHandler(finishedContext -> finishCount.incrementAndGet());

    assertEquals(3, context.getExpectedInstanceCount());
    assertEquals(1, context.getCallbackCount());

    context.finishWithStatus(LynxMemoryCollectionStatus.COMPLETED);
    context.addCallback(new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        callbackCount.incrementAndGet();
      }
    });
    context.didReceiveInstanceResult(createInstance(1, 1L, 1L, 1L, 1L, 1L, 1L, null));
    context.finishWithStatus(LynxMemoryCollectionStatus.TIMEOUT);

    assertEquals(1, callbackCount.get());
    assertEquals(1, finishCount.get());
    assertEquals(0, context.getCallbackCount());
    assertEquals(0, context.getReceivedFetchResultCount());
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(3, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
  }
}
