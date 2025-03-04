// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import static org.junit.Assert.*;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxViewBuilder;
import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxViewConfigProcessorTest {
  @Before
  public void setUp() {}

  @Test
  public void testParseSchemaConfig() {
    HashMap<String, String> schemaConfigMap = new HashMap<>();
    schemaConfigMap.put("auto_concurrency", "1");
    TestLynxViewBuilder lynxViewBuilder = new TestLynxViewBuilder();
    lynxViewBuilder.setLynxViewConfig(schemaConfigMap);
    LynxViewConfigProcessor.parseForLynxViewBuilder(schemaConfigMap, lynxViewBuilder);
    assertTrue(lynxViewBuilder.testEnableAutoConcurrency);

    schemaConfigMap.put("auto_concurrency", "0");
    LynxViewConfigProcessor.parseForLynxViewBuilder(schemaConfigMap, lynxViewBuilder);
    assertFalse(lynxViewBuilder.testEnableAutoConcurrency);

    schemaConfigMap.put("auto_concurrency", "ABC");
    LynxViewConfigProcessor.parseForLynxViewBuilder(schemaConfigMap, lynxViewBuilder);
    assertFalse(lynxViewBuilder.testEnableAutoConcurrency);
  }

  private static class TestLynxViewBuilder extends LynxViewBuilder {
    boolean testEnableAutoConcurrency;

    public TestLynxViewBuilder() {}

    @Override
    public LynxViewBuilder setEnableAutoConcurrency(boolean enable) {
      testEnableAutoConcurrency = enable;
      return super.setEnableAutoConcurrency(enable);
    }
  }
}
