// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.Looper;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class LynxElementTreeSerializerTest {
  @Test
  public void toJSONStringReturnsNullForInvalidNativeElement() throws Exception {
    LynxElement element = new LynxElement(null, 0);
    CountDownLatch latch = new CountDownLatch(1);
    final String[] result = new String[1];

    element.toJSONString(new LynxElement.Callback<String>() {
      @Override
      public void onResult(String json) {
        result[0] = json;
        latch.countDown();
      }
    });

    assertTrue(latch.await(3, TimeUnit.SECONDS));
    assertNull(result[0]);
  }

  @Test
  public void toJSONStringDoesNotInvokeCallbackSynchronouslyOnUiThread() throws Exception {
    LynxElement element = new LynxElement(null, 0);
    CountDownLatch callbackLatch = new CountDownLatch(1);
    AtomicBoolean methodReturned = new AtomicBoolean(false);
    AtomicBoolean callbackBeforeReturn = new AtomicBoolean(false);
    AtomicBoolean callbackOnUiThread = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        element.toJSONString(new LynxElement.Callback<String>() {
          @Override
          public void onResult(String json) {
            callbackBeforeReturn.set(!methodReturned.get());
            callbackOnUiThread.set(Thread.currentThread() == Looper.getMainLooper().getThread());
            callbackLatch.countDown();
          }
        });
        methodReturned.set(true);
      }
    });

    assertTrue(callbackLatch.await(3, TimeUnit.SECONDS));
    assertFalse(callbackBeforeReturn.get());
    assertTrue(callbackOnUiThread.get());
  }

  @Test
  public void getLynxElementRootReturnsNullAsynchronouslyWhenRenderIsNull() throws Exception {
    CountDownLatch callbackLatch = new CountDownLatch(1);
    AtomicBoolean methodReturned = new AtomicBoolean(false);
    AtomicBoolean callbackBeforeReturn = new AtomicBoolean(false);
    AtomicBoolean callbackOnUiThread = new AtomicBoolean(false);
    final LynxElement[] result = new LynxElement[1];

    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        LynxView lynxView =
            new LynxView(InstrumentationRegistry.getInstrumentation().getTargetContext());
        lynxView.getLynxElementRoot(new LynxElement.Callback<LynxElement>() {
          @Override
          public void onResult(LynxElement element) {
            result[0] = element;
            callbackBeforeReturn.set(!methodReturned.get());
            callbackOnUiThread.set(Thread.currentThread() == Looper.getMainLooper().getThread());
            callbackLatch.countDown();
          }
        });
        methodReturned.set(true);
      }
    });

    assertTrue(callbackLatch.await(3, TimeUnit.SECONDS));
    assertNull(result[0]);
    assertFalse(callbackBeforeReturn.get());
    assertTrue(callbackOnUiThread.get());
  }

  @Test
  public void nativeTreeJsonCallbackReturnsNullForEmptyString() throws Exception {
    CountDownLatch callbackLatch = new CountDownLatch(1);
    final String[] result = new String[1];
    List<LynxElement.NativeCallback> callbackHolders = new ArrayList<>();
    LynxElement.NativeCallback nativeCallback = new LynxElement.NativeCallback(
        LynxElement.REQUEST_TREE_JSON, null, new LynxElement.Callback<String>() {
          @Override
          public void onResult(String json) {
            result[0] = json;
            callbackLatch.countDown();
          }
        }, callbackHolders);

    callbackHolders.add(nativeCallback);
    nativeCallback.onSuccess("");

    assertTrue(callbackLatch.await(3, TimeUnit.SECONDS));
    assertNull(result[0]);
    assertTrue(callbackHolders.isEmpty());
  }

  @Test
  public void nativeCallbackCancelReturnsNullAndRemovesHolder() throws Exception {
    AtomicInteger callbackCount = new AtomicInteger(0);
    final Object[] result = new Object[1];
    List<LynxElement.NativeCallback> callbackHolders = new ArrayList<>();
    LynxElement.NativeCallback nativeCallback = new LynxElement.NativeCallback(
        LynxElement.REQUEST_TREE_JSON, null, new LynxElement.Callback<Object>() {
          @Override
          public void onResult(Object value) {
            result[0] = value;
            callbackCount.incrementAndGet();
          }
        }, callbackHolders);

    callbackHolders.add(nativeCallback);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        nativeCallback.cancel();
      }
    });
    nativeCallback.onSuccess("{\"tag\":\"page\"}");

    Thread.sleep(300);
    assertNull(result[0]);
    assertEquals(1, callbackCount.get());
    assertTrue(callbackHolders.isEmpty());
  }
}
