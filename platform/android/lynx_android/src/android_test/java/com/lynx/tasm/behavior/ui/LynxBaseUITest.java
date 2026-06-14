// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
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

  private void setEnableNewSticky(boolean enable) {
    JavaOnlyMap pageConfig = new JavaOnlyMap();
    pageConfig.putBoolean("enableNewSticky", enable);
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
  public void legacyStickyAcceptsFourFieldPayload() {
    setEnableNewSticky(false);
    UIView parent = new UIView(mContext);
    parent.setWidth(200);
    parent.setHeight(200);

    UIView sticky = new UIView(mContext);
    sticky.setLeft(0);
    sticky.setTop(100);
    sticky.setWidth(50);
    sticky.setHeight(40);
    parent.insertChild(sticky, 0);
    sticky.updateSticky(new float[] {0, 10, 0, 0});
    // Legacy sticky keeps consuming the 4-field payload: [left, top, right, bottom].
    // checkStickyOnParentScroll() should still update both mSticky and view translation.
    assertNotNull(sticky.mSticky);
    assertTrue(sticky.checkStickyOnParentScroll(0, 150));
    assertEquals(60.f, sticky.mSticky.y, 0.f);
    assertEquals(60.f, sticky.getView().getTranslationY(), 0.f);
  }

  @Test
  public void newStickyCalculateTranslateClampsToParentRange() {
    setEnableNewSticky(true);
    UIView sticky = new UIView(mContext);
    sticky.setLeft(0);
    sticky.setTop(100);
    sticky.setWidth(50);
    sticky.setHeight(40);
    sticky.updateSticky(new float[] {0, 10, 0, 0, 100, 200, 0, 100, 0, 50});
    // The 10-field payload includes parent size and relative offsets to the scroller.
    // Parent bottom clamps the translation to 110 instead of using the larger self-only range.
    assertNotNull(sticky.mSticky);
    assertTrue(sticky.calculateStickyTranslateWithOffset(250, true, 200, 300));
    assertEquals(110.f, sticky.mSticky.y, 0.f);
    assertEquals(110.f, sticky.getView().getTranslationY(), 0.f);
  }

  @Test
  public void newStickyInvalidPayloadClearsStickyStateAndTranslate() {
    setEnableNewSticky(true);
    UIView sticky = new UIView(mContext);
    sticky.setTop(100);
    sticky.setWidth(50);
    sticky.setHeight(40);

    // 1. First apply a valid 10-field payload, then pass 4-field sticky info.
    // The new sticky path should clear stale sticky state and view translation.
    sticky.updateSticky(new float[] {0, 10, 0, 0, -1, -1, 0, 100, 0, 0});
    assertTrue(sticky.calculateStickyTranslateWithOffset(150, true, 200, 300));
    assertEquals(60.f, sticky.getView().getTranslationY(), 0.f);

    sticky.updateSticky(new float[] {0, 10, 0, 0});
    assertNull(sticky.mSticky);
    assertEquals(0.f, sticky.getView().getTranslationY(), 0.f);

    // 2. Apply a valid 10-field payload, then pass null sticky info.
    // The new sticky path should clear stale sticky state and view translation.
    sticky.updateSticky(new float[] {0, 10, 0, 0, -1, -1, 0, 100, 0, 0});
    assertTrue(sticky.calculateStickyTranslateWithOffset(150, true, 200, 300));
    assertEquals(60.f, sticky.getView().getTranslationY(), 0.f);

    sticky.updateSticky(null);
    assertNull(sticky.mSticky);
    assertEquals(0.f, sticky.getView().getTranslationY(), 0.f);
  }
}
