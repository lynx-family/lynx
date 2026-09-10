// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIScrollViewPendingOffsetTest {
  private LynxContext mContext;
  private LynxUIOwner mOwner;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    mContext.setEventEmitter(mock(EventEmitter.class));
    UIBody body = TestingUtils.getUIBody(mContext);
    mOwner = TestingUtils.getLynxUIOwner(
        mContext, body.getBodyView(), new TestingUtils.BehaviorRegisterCallback() {});
    mContext.setLynxUIOwner(mOwner);
  }

  @Test
  public void customLayoutParentConsumesHorizontalOffsetOnce() {
    UIScrollView scroll = createScroller(false);
    UIView parent = new UIView(mContext) {
      @Override
      public boolean needCustomLayout() {
        return true;
      }
    };
    parent.setSign(2, "view");
    mOwner.setNode(2, parent);
    parent.insertChild(scroll, 0);
    scroll.setScrollLeftInner(55, false, false);
    layoutNative(scroll, false);
    assertEquals(0, scroll.getView().getRealScrollX());
    parent.layoutChildren();
    assertEquals(55, scroll.getView().getRealScrollX());
    scroll.getView().setScrollTo(90, 0, false);
    parent.layoutChildren();
    assertEquals(90, scroll.getView().getRealScrollX());
  }

  @Test
  public void verticalOffsetIsConsumedWithoutCallingUILayout() {
    UIScrollView scroll = createScroller(true);
    scroll.setScrollTopInner(55, false, false);
    layoutNative(scroll, true);
    scroll.layoutChildren();
    assertEquals(55, scroll.getView().getRealScrollY());
  }

  @Test
  public void zeroReplacesPendingOffset() {
    UIScrollView scroll = createScroller(false);
    scroll.setScrollLeftInner(55, false, false);
    scroll.setScrollLeftInner(0, false, false);
    layoutNative(scroll, false);
    scroll.getView().setScrollTo(90, 0, false);
    scroll.layoutChildren();
    assertEquals(0, scroll.getView().getRealScrollX());
    scroll.getView().setScrollTo(90, 0, false);
    scroll.layoutChildren();
    assertEquals(90, scroll.getView().getRealScrollX());
  }

  @Test
  public void logicalSizeAloneDoesNotConsumeOffset() {
    UIScrollView scroll = createScroller(false);
    scroll.setScrollLeftInner(55, false, false);
    scroll.getView().setMeasuredSize(600, 200);
    scroll.layoutChildren();
    assertEquals(0, scroll.getView().getRealScrollX());
    layoutNative(scroll, false);
    scroll.layoutChildren();
    assertEquals(55, scroll.getView().getRealScrollX());
  }

  @Test
  public void ordinaryLayoutStillConsumesOffset() {
    UIScrollView scroll = createScroller(false);
    scroll.setScrollLeftInner(55, false, false);
    layoutNative(scroll, false);
    scroll.layout();
    assertEquals(55, scroll.getView().getRealScrollX());
  }

  private UIScrollView createScroller(boolean vertical) {
    UIScrollView scroll = new UIScrollView(mContext);
    scroll.setSign(1, "scroll-view");
    mOwner.setNode(1, scroll);
    scroll.setWidth(200);
    scroll.setHeight(200);
    if (vertical) {
      scroll.setScrollY(true);
    } else {
      scroll.setScrollX(true);
    }
    return scroll;
  }

  private void layoutNative(UIScrollView scroll, boolean vertical) {
    AndroidScrollView view = scroll.getView();
    view.setMeasuredSize(vertical ? 200 : 600, vertical ? 600 : 200);
    view.layout(0, 0, 200, 200);
    view.getHScrollView().layout(0, 0, 200, vertical ? 600 : 200);
    view.getLinearLayout().layout(0, 0, vertical ? 200 : 600, vertical ? 600 : 200);
  }
}
