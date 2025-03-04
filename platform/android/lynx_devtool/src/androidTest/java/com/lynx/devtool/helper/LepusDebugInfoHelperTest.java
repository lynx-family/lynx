// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper;

import static org.junit.Assert.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LepusDebugInfoHelperTest {
  LepusDebugInfoHelper mDebugInfoHelper;

  @Before
  public void setUp() {
    mDebugInfoHelper = new LepusDebugInfoHelper();
  }

  @Test
  public void getDebugInfo() {
    String invalidUrl = "https://error-url/debug-info.json";

    String result = mDebugInfoHelper.getDebugInfo(invalidUrl);
    assertTrue(result.isEmpty());
    assertEquals(mDebugInfoHelper.getDebugInfoUrl(), invalidUrl);
  }
}
