// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.ILynxErrorReceiver;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;

public class ResourceLoaderTest {
  private static final int LYNX_RESOURCE_TYPE_EXTERNAL_BYTE_CODE = 16;

  private static class MockBytecodeErrorFetcher extends LynxGenericResourceFetcher {
    @Override
    public void fetchResource(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {}

    @Override
    public void fetchResourcePath(
        LynxResourceRequest request, LynxResourceCallback<String> callback) {}

    @Override
    public void fetchBytecode(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {
      callback.onResponse(LynxResourceResponse.onFailed(new Throwable("bytecode error")));
    }

    @Override
    public void cancel(LynxResourceRequest request) {}
  }

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }
  @Test
  public void testLoadLynxJSAsset() {
    ResourceLoader loader = new ResourceLoader();
    // file exists
    byte[] resource1 = loader.loadLynxJSAsset("lynx_assets://lynx_core.js");
    assertNotNull(resource1);
    assertTrue(resource1.length > 0);
    // file does not exist
    byte[] resource2 = loader.loadLynxJSAsset("lynx_assets://non-existing.js");
    assertNull(resource2);
  }

  @Test
  public void testLoadBytecodeFailureDoesNotReportError() throws Exception {
    CountDownLatch errorLatch = new CountDownLatch(1);
    ILynxErrorReceiver errorReceiver = new ILynxErrorReceiver() {
      @Override
      public void onErrorOccurred(LynxError error) {
        errorLatch.countDown();
      }
    };
    LynxResourceLoader loader =
        new LynxResourceLoader(null, null, errorReceiver, null, new MockBytecodeErrorFetcher());

    Method loadBytecode = LynxResourceLoader.class.getDeclaredMethod(
        "loadBytecode", long.class, String.class, int.class);
    loadBytecode.setAccessible(true);
    try {
      loadBytecode.invoke(loader, 0L, "bytecode_url", LYNX_RESOURCE_TYPE_EXTERNAL_BYTE_CODE);
    } catch (InvocationTargetException e) {
      if (!(e.getCause() instanceof UnsatisfiedLinkError)) {
        throw e;
      }
      // Some Java-only test environments do not load nativeInvokeCallback.
    }

    assertFalse(errorLatch.await(200, TimeUnit.MILLISECONDS));
  }
}
