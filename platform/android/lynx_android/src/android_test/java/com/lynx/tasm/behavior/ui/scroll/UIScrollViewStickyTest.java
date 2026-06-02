// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIScrollViewStickyTest {
  private LynxContext mContext;
  private LynxUIOwner mUIOwner;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    JavaOnlyMap pageConfig = new JavaOnlyMap();
    pageConfig.putBoolean("enableNewSticky", true);
    mContext.onPageConfigDecoded(new PageConfig(pageConfig));
    UIBody uiBody = TestingUtils.getUIBody(mContext);
    mUIOwner = TestingUtils.getLynxUIOwner(
        mContext, uiBody.getBodyView(), new TestingUtils.BehaviorRegisterCallback() {});
    mContext.setLynxUIOwner(mUIOwner);
  }

  // Focus: A valid new-sticky payload registers the sticky node sign on the nearest scroll-view.
  @Test
  public void newStickyRegistersWithScrollerBySign() throws Exception {
    UIScrollView scroller = createScroller();
    UIView sticky = createStickyChild();
    scroller.insertChild(sticky, 0);

    sticky.updateSticky(createVerticalStickyInfo(10));
    assertStickyChildSigns(scroller, sticky.getSign());
  }

  // Focus: Sticky translation is calculated from an explicit scroll offset, independent of Android
  // ScrollView layout state in unit tests.
  @Test
  public void newStickyCalculatesTranslateWithExplicitOffset() throws Exception {
    UIView sticky = createStickyChild();
    sticky.updateSticky(createVerticalStickyInfo(10));

    assertTrue(sticky.calculateStickyTranslateWithOffset(150, true, 200, 300));
    assertEquals(60.f, getStickyTranslateY(sticky), 0.f);
  }

  // Focus: Updating sticky info keeps one recorded sign for the same node, while invalid or null
  // payloads clear the node from the scroll-view's sticky sign array.
  @Test
  public void newStickyUpdatesStickyChildSignsWhenInfoChanges() throws Exception {
    UIScrollView scroller = createScroller();
    UIView sticky = createStickyChild();
    scroller.insertChild(sticky, 0);

    sticky.updateSticky(createVerticalStickyInfo(10));
    assertStickyChildSigns(scroller, sticky.getSign());

    sticky.updateSticky(createVerticalStickyInfo(20));
    assertStickyChildSigns(scroller, sticky.getSign());

    sticky.updateSticky(new float[] {0, 20, 0, 0});
    assertStickyChildSigns(scroller);

    sticky.updateSticky(createVerticalStickyInfo(30));
    assertStickyChildSigns(scroller, sticky.getSign());

    sticky.updateSticky(null);
    assertStickyChildSigns(scroller);
  }

  // Focus: Removing a sticky node unregisters it from the scroll-view sticky sign array, so later
  // sticky refreshes do not process the removed node.
  @Test
  public void newStickyUnregistersFromScrollerOnNodeRemoved() throws Exception {
    UIScrollView scroller = createScroller();
    UIView sticky = createStickyChild();
    scroller.insertChild(sticky, 0);

    sticky.updateSticky(createVerticalStickyInfo(10));
    assertStickyChildSigns(scroller, sticky.getSign());

    sticky.onNodeRemoved();
    scroller.removeChild(sticky);
    assertStickyChildSigns(scroller);
  }

  // Focus: Moving a sticky node between two scroll-views removes its sign from the old scroll-view
  // and records it on the new scroll-view after sticky info is updated.
  @Test
  public void newStickyMovesBetweenScrollersWhenStickyInfoUpdates() throws Exception {
    UIScrollView firstScroller = createScroller(1);
    UIScrollView secondScroller = createScroller(3);
    UIView sticky = createStickyChild(2);
    firstScroller.insertChild(sticky, 0);

    sticky.updateSticky(createVerticalStickyInfo(10));
    assertStickyChildSigns(firstScroller, sticky.getSign());
    assertStickyChildSigns(secondScroller);

    firstScroller.removeChild(sticky);
    secondScroller.insertChild(sticky, 0);
    sticky.updateSticky(createVerticalStickyInfo(10));

    assertStickyChildSigns(firstScroller);
    assertStickyChildSigns(secondScroller, sticky.getSign());
  }

  private UIScrollView createScroller() {
    return createScroller(1);
  }

  private UIScrollView createScroller(int sign) {
    UIScrollView scroller = new UIScrollView(mContext);
    scroller.setSign(sign, "scroll-view");
    scroller.setWidth(200);
    scroller.setHeight(200);
    scroller.setScrollY(true);
    scroller.getView().setMeasuredSize(200, 500);
    mUIOwner.setNode(scroller.getSign(), scroller);
    return scroller;
  }

  private UIView createStickyChild() {
    return createStickyChild(2);
  }

  private UIView createStickyChild(int sign) {
    UIView sticky = new UIView(mContext);
    sticky.setSign(sign, "view");
    sticky.setLeft(0);
    sticky.setTop(100);
    sticky.setWidth(50);
    sticky.setHeight(40);
    mUIOwner.setNode(sticky.getSign(), sticky);
    return sticky;
  }

  private float[] createVerticalStickyInfo(float top) {
    return new float[] {0, top, 0, 0, -1, -1, 0, 100, 0, 0};
  }

  private void assertStickyChildSigns(UIScrollView scroller, int... expectedSigns)
      throws Exception {
    ArrayList<Integer> expected = new ArrayList<>();
    for (int sign : expectedSigns) {
      expected.add(sign);
    }
    assertEquals(expected, getStickyChildSigns(scroller));
  }

  @SuppressWarnings("unchecked")
  private List<Integer> getStickyChildSigns(UIScrollView scroller) throws Exception {
    Field field = UIScrollView.class.getDeclaredField("mStickyChildSigns");
    field.setAccessible(true);
    ArrayList<Integer> signs = (ArrayList<Integer>) field.get(scroller);
    if (signs == null) {
      return Collections.emptyList();
    }
    return new ArrayList<>(signs);
  }

  private float getStickyTranslateY(UIView sticky) throws Exception {
    Field stickyField = LynxBaseUI.class.getDeclaredField("mSticky");
    stickyField.setAccessible(true);
    Object stickyInfo = stickyField.get(sticky);
    if (stickyInfo == null) {
      return 0.f;
    }
    Field yField = stickyInfo.getClass().getDeclaredField("y");
    yField.setAccessible(true);
    return yField.getFloat(stickyInfo);
  }
}
