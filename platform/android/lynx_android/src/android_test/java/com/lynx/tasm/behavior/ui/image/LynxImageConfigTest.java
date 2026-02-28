// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static com.lynx.tasm.base.Assertions.assertNotNull;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.image.LynxImageConfig;
import java.util.HashMap;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxImageConfigTest {
  private LynxViewBuilder viewBuilder;

  @Before
  public void setUp() {
    viewBuilder = new LynxViewBuilder();
  }

  @After
  public void tearDown() {}

  @Test
  public void testSetLynxImageConfig() {
    LynxImageConfig imageConfig = viewBuilder.getLynxImageConfig();
    assertNull(imageConfig);

    imageConfig = new LynxImageConfig();
    viewBuilder.setLynxImageConfig(imageConfig);
    LynxImageConfig config = viewBuilder.getLynxImageConfig();
    assertNotNull(config);
  }

  @Test
  public void testConfigValue() {
    LynxImageConfig imageConfig = new LynxImageConfig();
    imageConfig.setEnableProgressiveRendering(true);
    assertTrue(imageConfig.getEnableProgressiveRendering());

    imageConfig.setEnableImageResourceHint(true);
    assertTrue(imageConfig.getEnableImageResourceHint());

    imageConfig.setEnableImageSmallDiskCache(true);
    assertTrue(imageConfig.getEnableImageSmallDiskCache());

    HashMap<String, String> customParams = new HashMap<>();
    customParams.put("key1", "value1");
    customParams.put("key2", "value2");
    imageConfig.setImageCustomParam(customParams);
    assertEquals(customParams, imageConfig.getImageCustomParam());

    imageConfig.setEnableImageLoadCallback(true);
    assertTrue(imageConfig.getEnableImageLoadCallback());
  }
}
