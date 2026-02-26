// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import org.junit.Before;
import org.junit.Test;

public class LynxResourceModuleTest {
  private LynxResourceModule mModule;
  private LynxContext mLynxContext;

  @Before
  public void setUp() {
    mLynxContext = mock(LynxContext.class);
    mModule = new LynxResourceModule(mLynxContext);
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

    mModule.requestResourcePrefetch(requestData, null);

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

    mModule.requestResourcePrefetch(requestData, null);

    // On Android, it should still succeed even without font-family
    verify(mockFetcher, timeout(1000)).fetchResource(any(), any());
  }
}
