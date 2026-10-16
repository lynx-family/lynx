// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;

import java.lang.reflect.Method;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import org.junit.Test;

public class NativeFacadeProxyTest {
  @Test
  public void emptyProxyUsesDefaultRoutingAndDropsCallbacks() {
    NativeFacadeProxy proxy = new NativeFacadeProxy();
    assertTrue(proxy.shouldSendEventToMainThread());
    assertFalse(proxy.setTemplateLoadClient(new RecordingClient()));
    proxy.reportError(new LynxError(1, "detached"));
    assertNull(proxy.getNativeFacade());
    assertNull(proxy.detach());
  }

  @Test
  public void routingFollowsNativeDecisionWithoutDependingOnJavaScript() throws Exception {
    NativeFacadeProxy proxy = new NativeFacadeProxy();
    NativeFacade facade = new NativeFacade(false);
    proxy.attach(facade);
    assertTrue(proxy.shouldSendEventToMainThread());
    setNativeRouting(facade, false);
    assertFalse(proxy.shouldSendEventToMainThread());
    setNativeRouting(facade, true);
    assertTrue(proxy.shouldSendEventToMainThread());
  }

  @Test
  public void reloadKeepsOldCallbacksAndDestructionOnTheOldFacade() throws Exception {
    NativeFacadeProxy proxy = new NativeFacadeProxy();
    RecordingFacade first = new RecordingFacade();
    RecordingClient firstClient = new RecordingClient();
    proxy.attach(first);
    assertTrue(proxy.setTemplateLoadClient(firstClient));
    setNativeRouting(first, false);
    NativeFacade firstSnapshot = proxy.getNativeFacade();

    NativeFacade detached = proxy.detach();
    assertSame(first, detached);
    assertTrue(proxy.shouldSendEventToMainThread());
    assertFalse(proxy.setTemplateLoadClient(new RecordingClient()));
    proxy.reportError(new LynxError(1, "detached"));
    assertEquals(0, first.errorCount);
    assertNull(proxy.detach());

    RecordingFacade second = new RecordingFacade();
    RecordingClient secondClient = new RecordingClient();
    proxy.attach(second);
    assertTrue(proxy.setTemplateLoadClient(secondClient));
    firstSnapshot.reportError(new LynxError(1, "old callback"));
    detached.destroyAnyThreadPart();
    detached.destroyUiThreadPart();
    assertEquals(1, first.errorCount);
    assertEquals(0, second.errorCount);
    assertEquals(1, firstClient.destroyCount);
    assertEquals(0, secondClient.destroyCount);
    assertSame(second, proxy.getNativeFacade());
    assertTrue(proxy.shouldSendEventToMainThread());
    proxy.reportError(new LynxError(1, "new callback"));
    assertEquals(1, second.errorCount);
  }

  @Test(timeout = 10000)
  public void queryCanFinishWhileItsFacadeIsDetachedAndReplaced() throws Exception {
    NativeFacadeProxy proxy = new NativeFacadeProxy();
    CountDownLatch entered = new CountDownLatch(1);
    CountDownLatch release = new CountDownLatch(1);
    NativeFacade first = new NativeFacade(false) {
      @Override
      boolean shouldSendEventToMainThread() {
        entered.countDown();
        await(release);
        return false;
      }
    };
    proxy.attach(first);
    ExecutorService executor = Executors.newSingleThreadExecutor();
    try {
      Future<Boolean> query = executor.submit(proxy::shouldSendEventToMainThread);
      assertTrue(entered.await(5, TimeUnit.SECONDS));
      assertSame(first, proxy.detach());
      proxy.attach(new NativeFacade(false));
      release.countDown();
      assertFalse(query.get(5, TimeUnit.SECONDS));
      assertTrue(proxy.shouldSendEventToMainThread());
    } finally {
      release.countDown();
      executor.shutdownNow();
    }
  }

  @Test(timeout = 10000)
  public void clientCallbackDoesNotBlockDetachOrSwitchToTheReplacement() throws Exception {
    NativeFacadeProxy proxy = new NativeFacadeProxy();
    CountDownLatch entered = new CountDownLatch(1);
    CountDownLatch release = new CountDownLatch(1);
    RecordingFacade first = new RecordingFacade() {
      @Override
      public void reportError(LynxError error) {
        entered.countDown();
        await(release);
        super.reportError(error);
      }
    };
    RecordingFacade second = new RecordingFacade();
    proxy.attach(first);
    ExecutorService executor = Executors.newSingleThreadExecutor();
    try {
      Future<?> callback = executor.submit(() -> proxy.reportError(new LynxError(1, "in flight")));
      assertTrue(entered.await(5, TimeUnit.SECONDS));
      assertSame(first, proxy.detach());
      proxy.attach(second);
      release.countDown();
      callback.get(5, TimeUnit.SECONDS);
      assertEquals(1, first.errorCount);
      assertEquals(0, second.errorCount);
    } finally {
      release.countDown();
      executor.shutdownNow();
    }
  }

  private static void setNativeRouting(NativeFacade facade, boolean value) throws Exception {
    Method method =
        NativeFacade.class.getDeclaredMethod("onShouldSendEventToMainThreadChanged", boolean.class);
    method.setAccessible(true);
    method.invoke(facade, value);
  }

  private static void await(CountDownLatch latch) {
    try {
      assertTrue(latch.await(5, TimeUnit.SECONDS));
    } catch (InterruptedException exception) {
      Thread.currentThread().interrupt();
      throw new AssertionError(exception);
    }
  }

  private static class RecordingFacade extends NativeFacade {
    int errorCount;

    RecordingFacade() {
      super(false);
    }

    @Override
    public void reportError(LynxError error) {
      ++errorCount;
    }
  }

  private static class RecordingClient extends LynxViewClient {
    int destroyCount;

    @Override
    public void onDestroy() {
      ++destroyCount;
    }
  }
}
