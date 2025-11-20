// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import static org.junit.Assert.*;

import com.lynx.tasm.LynxEnv;
import java.lang.reflect.Field;
import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class FSPConfigTest {
  @Before
  public void setUp() throws Exception {}

  @Test
  public void testParseWithEnabledFSPAndConfig() {
    HashMap<String, String> testConfig = new HashMap<>();
    testConfig.put("min_content_fill_percentage_x", "40");
    testConfig.put("min_content_fill_percentage_y", "50");
    testConfig.put("min_content_fill_percentage_total_area", "60");
    testConfig.put("acceptable_pixel_diff_per_sec", "20");
    testConfig.put("acceptable_area_diff_per_sec", "200");
    testConfig.put("min_diff_interval_ms", "400");
    testConfig.put("hard_timeout_ms", "15000");
    testConfig.put("snapshotIntervalMs", "20");
    try {
      setEnableFSPField(true);
      setConfigField(testConfig);
      FSPConfig config = new FSPConfig();
      // parse config
      config.parse();
      // verify config values
      assertTrue(config.enable);
      assertEquals(40, config.minContentFillPercentageX);
      assertEquals(50, config.minContentFillPercentageY);
      assertEquals(60, config.minContentFillPercentageTotalArea);
      assertEquals(20, config.acceptablePixelDiffPerSec);
      assertEquals(200, config.acceptableAreaDiffPerSec);
      assertEquals(400, config.minDiffIntervalMs);
      assertEquals(15000, config.hardTimeoutMs);
      assertEquals(20, config.snapshotIntervalMs);
    } catch (Exception e) {
    }
  }
  private void setConfigField(HashMap<String, String> testConfig) throws Exception {
    Field field = LynxEnv.class.getDeclaredField("mFSPConfig");
    field.setAccessible(true);
    field.set(LynxEnv.inst(), testConfig);
  }

  private void setEnableFSPField(boolean value) throws Exception {
    Field field = LynxEnv.class.getDeclaredField("mEnableFSP");
    field.setAccessible(true);
    field.set(LynxEnv.inst(), value);
  }
}
