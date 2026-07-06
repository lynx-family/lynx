// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertNotNull;

import android.app.Application;
import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.provider.LynxResRequest;
import com.lynx.tasm.provider.LynxResResponse;
import com.lynx.tasm.resourceprovider.TestContentProvider;
import com.lynx.tasm.utils.StringUtils;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;

public class ResManagerTest {
  private static final String CONTENT_URI = "content://com.lynx.test.resourceprovider/resource";

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  @Test
  public void requestResourceSyncReturnsContentProviderBytes() {
    byte[] expected = "Hello Lynx ResManager ContentProvider".getBytes(StandardCharsets.UTF_8);
    TestContentProvider.setContent(expected);

    LynxResResponse response =
        ResManager.inst().requestResourceSync(new LynxResRequest(CONTENT_URI, null));

    assertNotNull(response);
    assertNotNull(response.getInputStream());
    assertArrayEquals(expected, StringUtils.streamToBytes(response.getInputStream()));
  }
}
