// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import static org.junit.Assert.*;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import org.junit.Test;

public class GlobalRefQueueTest {
  @Test
  public void loopPush() {
    final int initCapacity = 4;
    final GlobalRefQueue refQueue = new GlobalRefQueue();
    final int[] refHashCodes = new int[initCapacity];
    final long[] refQueueIds = new long[initCapacity];

    final int testLoop = 10;
    // test loop
    for (int l = 0; l < testLoop; ++l) {
      for (int i = 0; i < initCapacity; ++i) {
        Object ref = new Integer(i);
        refHashCodes[i] = ref.hashCode();
        refQueueIds[i] = refQueue.push(ref);
        assertTrue(refQueueIds[i] >= 0);
      }
      for (int i = 0; i < initCapacity; ++i) {
        Integer result_obj = (Integer) refQueue.pop(refQueueIds[i]);
        assertEquals(refHashCodes[i], result_obj.hashCode());
      }
      // test return null if id does not exist
      for (int i = 0; i < initCapacity; ++i) {
        assertNull(refQueue.pop(refQueueIds[i]));
      }
    }

    // test push null
    long refId = refQueue.push(null);
    assertTrue(refId < 0);
    Object o = refQueue.pop(refId);
    assertNull(o);
  }

  @Test
  public void reversePop() {
    final int initCapacity = 4;
    final GlobalRefQueue refQueue = new GlobalRefQueue();
    final int[] refHashCodes = new int[initCapacity];
    final long[] refQueueIds = new long[initCapacity];

    for (int i = 0; i < initCapacity; ++i) {
      Object ref = new Integer(i);
      refHashCodes[i] = ref.hashCode();
      refQueueIds[i] = refQueue.push(ref);
      assertTrue(refQueueIds[i] >= 0);
    }
    // test reverse pop objects
    for (int i = initCapacity - 1; i >= 0; --i) {
      Integer result_obj = (Integer) refQueue.pop(refQueueIds[i]);
      assertEquals(refHashCodes[i], result_obj.hashCode());
    }
    // test return null if id does not exist
    for (int i = 0; i < initCapacity; ++i) {
      assertNull(refQueue.pop(refQueueIds[i]));
    }
  }

  private static class VolatileInt {
    public volatile int mInt = 0;
  };

  @Test
  public void multiProducerMultiConsumer() {
    for (int i = 0; i < 100; i++) {
      multiProducerMultiConsumerImpl();
    }
  }

  private void multiProducerMultiConsumerImpl() {
    final Lock initedThreadLock = new ReentrantLock();
    final Condition initedThreadCondition = initedThreadLock.newCondition();

    final int producerCount = 2;
    final int consumerCount = 8;
    final CountDownLatch initedThreadLatch = new CountDownLatch(producerCount + consumerCount);

    final GlobalRefQueue refQueue = new GlobalRefQueue();
    final int maxProductCount = 51200;
    final VolatileInt[] refHashCodes = new VolatileInt[maxProductCount];
    for (int i = 0; i < maxProductCount; ++i) {
      refHashCodes[i] = new VolatileInt();
    }

    final ConcurrentLinkedQueue<Long> productIdQueue = new ConcurrentLinkedQueue<>();

    final AtomicInteger productIndexGenerator = new AtomicInteger(0);
    final Thread[] producers = new Thread[producerCount];
    for (int t = 0; t < producerCount; ++t) {
      producers[t] = new Thread(() -> {
        try {
          initedThreadLock.lock();
          initedThreadLatch.countDown();
          initedThreadCondition.await();
          initedThreadLock.unlock();

          int productIndex = productIndexGenerator.get();
          while (productIndex < maxProductCount) {
            if (productIndexGenerator.weakCompareAndSet(productIndex, productIndex + 1)) {
              Integer product = new Integer(productIndex);
              // keep hashCode of product to verify in consumer
              refHashCodes[productIndex].mInt = product.hashCode();
              long productRefId = refQueue.push(product);
              assertTrue(productRefId >= 0);
              // pass id instead of ref as product to consumer
              productIdQueue.offer(productRefId);
            }
            Thread.yield();
            productIndex = productIndexGenerator.get();
          }
        } catch (InterruptedException e) {
          assertTrue(false);
        }
      });
      producers[t].start();
    }

    final AtomicInteger consumeCountDown = new AtomicInteger(maxProductCount);
    final Thread[] consumers = new Thread[consumerCount];
    for (int t = 0; t < consumerCount; ++t) {
      consumers[t] = new Thread(() -> {
        try {
          initedThreadLock.lock();
          initedThreadLatch.countDown();
          initedThreadCondition.await();
          initedThreadLock.unlock();

          while (consumeCountDown.get() > 0) {
            Long productRefId = productIdQueue.poll();
            if (productRefId == null) {
              Thread.yield();
              continue;
            }
            consumeCountDown.decrementAndGet();
            // retrive product by id
            Integer product = (Integer) refQueue.pop(productRefId);
            assertNotNull(product);
            // test by hashCode
            assertEquals(refHashCodes[product.intValue()].mInt, product.hashCode());
            Thread.yield();
          }
        } catch (InterruptedException e) {
          assertTrue(false);
        }
      });
      consumers[t].start();
    }

    try {
      initedThreadLatch.await();
      initedThreadLock.lock();
      initedThreadCondition.signalAll();
      initedThreadLock.unlock();

      for (int t = 0; t < producerCount; ++t) {
        producers[t].join(t == 0 ? 2_000 : 100);
      }
      for (int t = 0; t < consumerCount; ++t) {
        consumers[t].join(t == 0 ? 3_000 : 100);
      }
    } catch (InterruptedException e) {
      assertTrue(false);
    }
    assertEquals(maxProductCount, productIndexGenerator.get());
    assertEquals(0, consumeCountDown.get());
  }
}
