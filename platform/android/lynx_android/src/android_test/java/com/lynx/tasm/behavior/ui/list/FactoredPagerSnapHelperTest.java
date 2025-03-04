// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import androidx.recyclerview.widget.RecyclerView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class FactoredPagerSnapHelperTest {
  private LynxContext lynxContext;
  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
  }
  @Test
  public void testSnap() {
    FactoredPagerSnapHelper helper = new FactoredPagerSnapHelper();
    RecyclerView view = new RecyclerView(lynxContext);
    helper.attachToRecyclerView(view);

    helper.mPagerHooks = new FactoredPagerSnapHelper.FactoredPagerHooks() {
      @Override
      public void willSnapTo(
          int position, int currentOffsetX, int currentOffsetY, int distanceX, int distanceY) {}
    };
    helper.setPagerAlignFactor(0);
    helper.setPagerAlignOffset(20);
  }
}
