// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.graphics.Matrix;
import android.graphics.Rect;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxBaseUITest {
  private LynxContext mContext;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
  }

  @After
  public void tearDown() throws Exception {}

  private void setEnableNativeInteraction(boolean enable) {
    JavaOnlyMap pageConfig = new JavaOnlyMap();
    pageConfig.putBoolean("enableNativeInteraction", enable);
    mContext.onPageConfigDecoded(new PageConfig(pageConfig));
  }

  @Test
  public void getTargetPoint() throws NoSuchFieldException, IllegalAccessException {
    try {
      LynxBaseUI ui = new UIView(mContext);
      float[] point = ui.getTargetPoint(0, 0, 0, 0, new Rect(0, 0, 0, 0), new Matrix());
      assertEquals(0, point[0], 0);
      assertEquals(0, point[1], 0);

      Matrix m = new Matrix();
      m.setScale(0, 0);
      point = ui.getTargetPoint(0, 0, 0, 0, new Rect(0, 0, 0, 0), m);
      assertEquals(Float.MAX_VALUE, point[0], 0);
      assertEquals(Float.MAX_VALUE, point[1], 0);
    } catch (Throwable e) {
      e.printStackTrace();
      assertEquals(1, 0, 0);
    }
  }

  @Test
  public void nativeInteractionDefaultsToPageConfigWhenPropUnset() {
    setEnableNativeInteraction(true);

    UIView ui = new UIView(mContext);

    assertTrue(ui.nativeInteractionEnabled);
  }

  @Test
  public void displayNoneAffectsVisibility() {
    LynxBaseUI ui = new UIView(mContext);
    assertTrue(ui.getVisibility());

    ui.setDisplayNone(true);
    assertFalse(ui.getVisibility());

    ui.setDisplayNone(false);
    assertTrue(ui.getVisibility());
  }
}
