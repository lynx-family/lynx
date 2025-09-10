// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.UIImage;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller;
import com.lynx.testing.base.TestingUtils;
import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxUIScrollViewTest {
  private LynxContext mContext;
  private LynxUIScrollView mUIScrollView;
  private LynxUIOwner mLynxUIOwner;
  private UIBody mUIBody;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    LynxTemplateRender renderer = mock(LynxTemplateRender.class);
    mContext.setEventEmitter(new LynxEventEmitter(renderer.getEngineProxy()));
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mLynxUIOwner = TestingUtils.getLynxUIOwner(
        mContext, mUIBody.getBodyView(), new TestingUtils.BehaviorRegisterCallback() {});
    mContext.setLynxUIOwner(mLynxUIOwner);
    LynxView mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mUIScrollView = new LynxUIScrollView(mContext);

    // Set some initial size for the scroll view
    mUIScrollView.setLeft(0);
    mUIScrollView.setTop(0);
    mUIScrollView.setWidth(200);
    mUIScrollView.setHeight(200);

    // Add some children to make it scrollable
    for (int i = 0; i < 5; i++) {
      UIImage child = new UIImage(mContext);
      child.setLeft(0);
      child.setTop(i * 100);
      child.setWidth(200);
      child.setHeight(100);
      mUIScrollView.insertChild(child, i);
    }
    mUIScrollView.measure();
    mUIScrollView.onNodeReady();
  }

  @Test
  public void setScrollOrientation() {
    mUIScrollView.setScrollOrientation("horizontal");
    assertFalse(mUIScrollView.getView().mIsVertical);
    mUIScrollView.setScrollOrientation("vertical");
    assertTrue(mUIScrollView.getView().mIsVertical);
  }

  @Test
  public void setEnableScroll() {
    mUIScrollView.setEnableScroll(false);
    assertFalse(mUIScrollView.getView().scrollEnabled());
    mUIScrollView.setEnableScroll(true);
    assertTrue(mUIScrollView.getView().scrollEnabled());
  }

  @Test
  public void setBounces() {
    mUIScrollView.setBounces(false);
    assertFalse(mUIScrollView.getView().bounces());
    mUIScrollView.setBounces(true);
    assertTrue(mUIScrollView.getView().bounces());
  }

  @Test
  public void setForwardsNestedScroll() {
    mUIScrollView.setForwardsNestedScroll(LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARALLEL);
    assertEquals(LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARALLEL,
        mUIScrollView.getView().getForwardNestedScrollMode());
  }

  @Test
  public void setBackwardsNestedScroll() {
    mUIScrollView.setBackwardsNestedScroll(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_FIRST);
    assertEquals(LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_FIRST,
        mUIScrollView.getView().getBackwardNestedScrollMode());
  }

  @Test
  public void setInitialScrollIndex() {
    // Re-create the UI to test initial scroll
    LynxUIScrollView scrollView = new LynxUIScrollView(mContext);
    for (int i = 0; i < 5; i++) {
      UIImage child = new UIImage(mContext);
      child.setLeft(0);
      child.setTop(i * 100);
      child.setWidth(200);
      child.setHeight(100);
      scrollView.insertChild(child, i);
    }
    scrollView.setInitialScrollIndex(2);
    scrollView.measure();
    scrollView.onNodeReady();
    assertEquals(200, scrollView.getScrollY());
  }

  @Test
  public void setInitialScrollOffset() {
    // Re-create the UI to test initial scroll
    LynxUIScrollView scrollView = new LynxUIScrollView(mContext);
    scrollView.setInitialScrollOffset("150px");
    scrollView.onNodeReady();
    scrollView.measure();
  }

  @Test
  public void setLowerThreshold() {
    mUIScrollView.setLowerThreshold("50px");
    // This would normally be verified by checking if a "scrolltolower" event is sent.
    // For this test, we just ensure it doesn't crash.
  }

  @Test
  public void setUpperThreshold() {
    mUIScrollView.setUpperThreshold("50px");
    // This would normally be verified by checking if a "scrolltoupper" event is sent.
    // For this test, we just ensure it doesn't crash.
  }

  @Test
  public void setScrollEventThrottle() {
    mUIScrollView.setScrollEventThrottle(500);
    // This would normally be verified by checking the frequency of "scroll" events.
    // For this test, we just ensure it doesn't crash.
  }

  @Test
  public void scrollTo() {
    JavaOnlyMap params = new JavaOnlyMap();
    params.putInt("index", 3);
    params.putBoolean("animated", false);
    mUIScrollView.scrollTo(params, mockSuccessCallback());
    assertEquals(300, mUIScrollView.getScrollY());
  }

  @Test
  public void scrollBy() {
    mUIScrollView.getView().scrollTo(0, 100);
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("offset", "50px");
    mUIScrollView.scrollBy(params, mockSuccessCallback());
  }

  @Test
  public void autoScroll() {
    JavaOnlyMap params = JavaOnlyMap.from(new HashMap());
    params.putString("rate", "600px"); // 600px per second
    // In a test environment without a running looper, we can't easily verify the animation.
    // We can, however, ensure that stopping the scroll works.
    params.putBoolean("start", false);
    mUIScrollView.autoScroll(params, mockSuccessCallback());
    // After stopping, the scroll state should eventually return to idle.
    // Direct assertion is difficult, but we ensure the call doesn't crash.
  }

  private Callback mockSuccessCallback() {
    return args -> assertEquals(LynxUIMethodConstants.SUCCESS, args[0]);
  }
}
