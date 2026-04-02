// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.LynxServiceCenter;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.stubbing.Answer;

@RunWith(AndroidJUnit4.class)
public class LynxResourceModuleTest {
  private LynxResourceModule mModule;
  private LynxContext mLynxContext;
  private ILynxImageService mImageService;

  @Before
  public void setUp() {
    LynxServiceCenter.inst().unregisterAllService();
    mLynxContext = mock(LynxContext.class);
    doAnswer((Answer<Void>) invocation -> {
      Runnable runnable = invocation.getArgument(0);
      if (runnable != null) {
        runnable.run();
      }
      return null;
    })
        .when(mLynxContext)
        .runOnJSThread(any(Runnable.class));
    mImageService = mock(ILynxImageService.class);
    doReturn((Class) ILynxImageService.class).when(mImageService).getServiceClass();
    LynxServiceCenter.inst().registerService(mImageService);
    mModule = new LynxResourceModule(mLynxContext);
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterAllService();
  }

  @Test
  public void testRequestResourcePrefetchFont() {
    String uri = "http://example.com/font.ttf";
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("font-family", "MyFont");

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "font");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    // Mock Fetcher
    LynxGenericResourceFetcher mockFetcher = mock(LynxGenericResourceFetcher.class);
    when(mLynxContext.getGenericResourceFetcher()).thenReturn(mockFetcher);

    mModule.requestResourcePrefetch(requestData, null, null);

    // Verify async call with timeout (to account for thread pool execution)
    verify(mockFetcher, timeout(1000)).fetchResource(any(), any());
  }

  @Test
  public void testRequestResourcePrefetchFontMissingFamily() {
    String uri = "http://example.com/font.ttf";
    // Missing font-family in params
    JavaOnlyMap params = new JavaOnlyMap();

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "font");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    // Mock Fetcher
    LynxGenericResourceFetcher mockFetcher = mock(LynxGenericResourceFetcher.class);
    when(mLynxContext.getGenericResourceFetcher()).thenReturn(mockFetcher);

    mModule.requestResourcePrefetch(requestData, null, null);

    // On Android, it should still succeed even without font-family
    verify(mockFetcher, timeout(1000)).fetchResource(any(), any());
  }

  @Test
  public void testRequestResourcePrefetchAwaitCompleteVideo() {
    String uri = "http://example.com/video.mp4";
    String preloadKey = "video_preload_key";
    String videoId = "video_id";
    long size = 2048L;
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("preloadKey", preloadKey);
    params.putString("videoID", videoId);
    params.putLong("size", size);

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "video");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 300L);

    ILynxResourceService mockResourceService = mock(ILynxResourceService.class);
    doReturn((Class) ILynxResourceService.class).when(mockResourceService).getServiceClass();
    LynxServiceCenter.inst().registerService(mockResourceService);
    doAnswer((Answer<Void>) invocation -> {
      ILynxResourceService.PreloadMediaCallback callback = invocation.getArgument(4);
      callback.onComplete(LynxSubErrorCode.E_SUCCESS, "");
      return null;
    })
        .when(mockResourceService)
        .preloadMedia(eq(uri), eq(preloadKey), eq(videoId), eq(size), any());

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertEquals(LynxSubErrorCode.E_SUCCESS, callbackResult[0].getInt("code"));
    ReadableArray details = callbackResult[0].getArray("details");
    Assert.assertNotNull(details);
    Assert.assertEquals(1, details.size());
    ReadableMap detail = details.getMap(0);
    Assert.assertEquals(uri, detail.getString("uri"));
    Assert.assertEquals("video", detail.getString("type"));
    Assert.assertEquals(LynxSubErrorCode.E_SUCCESS, detail.getInt("code"));
    Assert.assertEquals("", detail.getString("msg"));
  }

  @Test
  public void testRequestResourcePrefetchAwaitTimeoutVideo() {
    String uri = "http://example.com/video.mp4";
    String preloadKey = "video_preload_key";
    String videoId = "video_id";
    long size = 2048L;
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("preloadKey", preloadKey);
    params.putString("videoID", videoId);
    params.putLong("size", size);

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "video");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 50L);

    ILynxResourceService mockResourceService = mock(ILynxResourceService.class);
    doReturn((Class) ILynxResourceService.class).when(mockResourceService).getServiceClass();
    LynxServiceCenter.inst().registerService(mockResourceService);

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertEquals(
        LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT, callbackResult[0].getInt("code"));
    Assert.assertEquals("The prefetch task did not complete within the specified timeout.",
        callbackResult[0].getString("msg"));
    ReadableArray details = callbackResult[0].getArray("details");
    Assert.assertNotNull(details);
    Assert.assertEquals(0, details.size());
  }

  @Test
  public void testRequestResourcePrefetchAwaitCompleteImage() {
    String uri = "http://example.com/image.png";
    JavaOnlyMap params = new JavaOnlyMap();

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "image");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 300L);

    doAnswer((Answer<Void>) invocation -> {
      ImageLoadListener loadListener = invocation.getArgument(3);
      if (loadListener != null) {
        loadListener.onSuccess(null, null, null);
      }
      return null;
    })
        .when(mImageService)
        .prefetchImage(eq(uri), any(), any(), any());

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertEquals(LynxSubErrorCode.E_SUCCESS, callbackResult[0].getInt("code"));
    ReadableArray details = callbackResult[0].getArray("details");
    Assert.assertNotNull(details);
    Assert.assertEquals(1, details.size());
    ReadableMap detail = details.getMap(0);
    Assert.assertEquals(uri, detail.getString("uri"));
    Assert.assertEquals("image", detail.getString("type"));
    Assert.assertEquals(LynxSubErrorCode.E_SUCCESS, detail.getInt("code"));
  }

  @Test
  public void testRequestResourcePrefetchAwaitTimeoutImage() {
    String uri = "http://example.com/image.png";
    JavaOnlyMap params = new JavaOnlyMap();

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "image");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 50L);

    doAnswer((Answer<Void>) invocation -> null)
        .when(mImageService)
        .prefetchImage(eq(uri), any(), any(), any());

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertEquals(
        LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT, callbackResult[0].getInt("code"));
  }

  @Test
  public void testRequestResourcePrefetchAwaitCompleteFont() {
    String uri = "data:font/ttf;base64,@@@";
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("font-family", "MyFont");

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "font");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 300L);

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertNotEquals(
        LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT, callbackResult[0].getInt("code"));
    ReadableArray details = callbackResult[0].getArray("details");
    Assert.assertNotNull(details);
    Assert.assertEquals(1, details.size());
    ReadableMap detail = details.getMap(0);
    Assert.assertEquals("font", detail.getString("type"));
  }

  @Test
  public void testRequestResourcePrefetchAwaitTimeoutFont() {
    String uri = "http://example.com/font.ttf";
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("font-family", "MyFont");

    JavaOnlyMap item = new JavaOnlyMap();
    item.putString("uri", uri);
    item.putString("type", "font");
    item.putMap("params", params);

    JavaOnlyArray data = new JavaOnlyArray();
    data.pushMap(item);

    JavaOnlyMap requestData = new JavaOnlyMap();
    requestData.putArray("data", data);

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("awaitComplete", true);
    config.putLong("awaitTimeout", 50L);

    LynxGenericResourceFetcher mockFetcher = mock(LynxGenericResourceFetcher.class);
    when(mLynxContext.getGenericResourceFetcher()).thenReturn(mockFetcher);
    doAnswer((Answer<Void>) invocation -> null).when(mockFetcher).fetchResource(any(), any());

    Callback callback = mock(Callback.class);
    final JavaOnlyMap[] callbackResult = new JavaOnlyMap[1];
    doAnswer((Answer<Void>) invocation -> {
      callbackResult[0] = (JavaOnlyMap) invocation.getArgument(0);
      return null;
    })
        .when(callback)
        .invoke(any());

    mModule.requestResourcePrefetch(requestData, callback, config);

    verify(callback, timeout(1000)).invoke(any());
    Assert.assertNotNull(callbackResult[0]);
    Assert.assertEquals(
        LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT, callbackResult[0].getInt("code"));
  }
}
