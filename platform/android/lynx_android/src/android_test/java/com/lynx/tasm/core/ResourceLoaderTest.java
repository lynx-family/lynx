// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import org.junit.Before;
import org.junit.Test;

public class ResourceLoaderTest {
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
}
