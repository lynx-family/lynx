// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class HSRScriptSourceFetcherTest {
  private static final String KEY = LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS;
  private LynxResourceProvider mPreviousProvider;

  @Before
  public void setUp() {
    mPreviousProvider = LynxEnv.inst().getResourceProvider().remove(KEY);
  }

  @After
  public void tearDown() {
    LynxEnv.inst().getResourceProvider().remove(KEY);
    if (mPreviousProvider != null) {
      LynxEnv.inst().addResourceProvider(KEY, mPreviousProvider);
    }
  }

  @Test
  public void forwardsUrlsAndSourceBytesWithoutDecoding() throws Exception {
    String[] urls = {"https://example.com/script.js?q=%E4%B8%AD", "file:///tmp/a%20b.js"};
    byte[] source =
        "globalThis.text = '\u4e2d\u6587\ud83d\ude00';\u0000".getBytes(StandardCharsets.UTF_8);
    for (String url : urls) {
      FakeProvider provider = install((request, callback) -> {
        assertEquals(url, request.getUrl());
        assertEquals(LynxResourceRequest.LynxResourceType.LynxResourceTypeExternalJSSource,
            request.getRequestResourceType());
        callback.onResponse(LynxResourceResponse.success(source));
      });
      Result result = fetch(url);
      provider.await();
      assertNotEquals(Thread.currentThread(), provider.mThread);
      assertArrayEquals(source, result.mSource);
      assertNull(result.mError);
    }
  }

  @Test
  public void acceptsEmptyScript() throws Exception {
    install((request, callback) -> callback.onResponse(LynxResourceResponse.success(new byte[0])));
    Result result = fetch("file:///tmp/empty.js");
    assertArrayEquals(new byte[0], result.mSource);
    assertNull(result.mError);
  }

  @Test
  public void reportsMissingProviderAndMissingData() throws Exception {
    assertEquals("HSR external JS resource provider is not configured", fetch("url").mError);
    install((request, callback) -> callback.onResponse(null));
    assertEquals("HSR source provider returned no script data", fetch("url").mError);
    install((request, callback) -> callback.onResponse(LynxResourceResponse.success(null)));
    assertEquals("HSR source provider returned no script data", fetch("url").mError);
  }

  @Test
  public void reportsProviderErrorAndSynchronousException() throws Exception {
    install((request, callback)
                -> callback.onResponse(
                    LynxResourceResponse.failed(-1, new IllegalStateException("not found"))));
    assertEquals("Failed to load HSR script: not found", fetch("url").mError);
    install((request, callback) -> { throw new IllegalStateException(); });
    assertEquals("Failed to load HSR script: IllegalStateException", fetch("url").mError);
  }

  @Test
  public void completesOnceWhenProviderRepeatsCallbackAndThrows() throws Exception {
    FakeProvider provider = install((request, callback) -> {
      callback.onResponse(LynxResourceResponse.success(new byte[] {42}));
      callback.onResponse(LynxResourceResponse.failed(-1, new IllegalStateException("late")));
      throw new IllegalStateException("after callback");
    });
    Result result = fetch("url");
    provider.await();
    assertEquals(1, result.mCount.get());
    assertArrayEquals(new byte[] {42}, result.mSource);
    assertNull(result.mError);
  }

  private FakeProvider install(Action action) {
    FakeProvider provider = new FakeProvider(action);
    LynxEnv.inst().addResourceProvider(KEY, provider);
    return provider;
  }

  private Result fetch(String url) throws Exception {
    Result result = new Result();
    HSRScriptSourceFetcher.fetch(url, (source, error) -> {
      result.mCount.incrementAndGet();
      result.mSource = source;
      result.mError = error;
      result.mDone.countDown();
    });
    assertTrue(result.mDone.await(5, TimeUnit.SECONDS));
    return result;
  }

  private interface Action {
    void request(LynxResourceRequest<Object> request, LynxResourceCallback<byte[]> callback);
  }

  private static class FakeProvider extends LynxResourceProvider<Object, byte[]> {
    private final Action mAction;
    private final CountDownLatch mDone = new CountDownLatch(1);
    private Thread mThread;

    FakeProvider(Action action) {
      mAction = action;
    }

    @Override
    public void request(
        LynxResourceRequest<Object> request, LynxResourceCallback<byte[]> callback) {
      mThread = Thread.currentThread();
      try {
        mAction.request(request, callback);
      } finally {
        mDone.countDown();
      }
    }

    void await() throws Exception {
      assertTrue(mDone.await(5, TimeUnit.SECONDS));
    }
  }

  private static class Result {
    private final CountDownLatch mDone = new CountDownLatch(1);
    private final AtomicInteger mCount = new AtomicInteger();
    private byte[] mSource;
    private String mError;
  }
}
