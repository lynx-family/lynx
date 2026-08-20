// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_RTL;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.scroll.utils.ScrollSnapHelper;
import com.lynx.tasm.behavior.ui.scroll.utils.ScrollSnapHelper.ScrollContainerHooks;
import com.lynx.tasm.behavior.ui.scroll.utils.ScrollSnapHelper.SnapTarget;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class UIScrollViewSnapTest {
  private static final int VIEWPORT_SIZE = 200;
  private static final int ITEM_SIZE = 100;
  private static final int ITEM_COUNT = 5;

  private LynxContext mContext;
  private LynxUIOwner mUIOwner;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    mContext.setEventEmitter(mock(EventEmitter.class));
    UIBody uiBody = TestingUtils.getUIBody(mContext);
    mUIOwner = TestingUtils.getLynxUIOwner(
        mContext, uiBody.getBodyView(), new TestingUtils.BehaviorRegisterCallback() {});
    mContext.setLynxUIOwner(mUIOwner);
  }

  @Test
  public void testItemSnapPropertyCreatesAndClearsHelper() throws Exception {
    // Verify that only non-empty item-snap properties create a helper, while null or empty
    // properties restore the original scrolling behavior.
    UIScrollView scroller = createVerticalScroller();
    JavaOnlyMap params = createSnapParams(0, 1);

    scroller.setItemSnap(params);
    assertNotNull(getScrollSnapHelper(scroller.getView()));

    scroller.setItemSnap(new JavaOnlyMap());
    assertNull(getScrollSnapHelper(scroller.getView()));

    scroller.setItemSnap(params);
    scroller.setItemSnap(null);
    assertNull(getScrollSnapHelper(scroller.getView()));
  }

  @Test
  public void testVerticalSnapHooksExposeNormalizedGeometry() throws Exception {
    // Verify that a vertical scroll-view provides the current offset, scroll range, viewport
    // size, and vertical child geometry to the helper.
    UIScrollView scroller = createVerticalScroller();
    scroller.getView().setScrollTo(0, 130, false);
    scroller.setItemSnap(createSnapParams(0, 1));

    ScrollSnapHelper helper = getScrollSnapHelper(scroller.getView());
    ScrollContainerHooks hooks = getScrollContainerHooks(helper);
    SnapTarget target = helper.findSnapTarget(0);

    assertEquals(130, hooks.getCurrentOffset());
    assertEquals(0, hooks.getMinOffset());
    assertEquals(300, hooks.getMaxOffset());
    assertEquals(VIEWPORT_SIZE, hooks.getViewportSize());
    assertEquals(ITEM_COUNT, hooks.getSnapItems().size());
    assertNull(hooks.getAdjacentSnapItem(1, true));
    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);
  }

  @Test
  public void testHorizontalLtrSnapUsesChildLeft() throws Exception {
    // Verify that a horizontal LTR scroll-view uses child.left as the increasing logical start
    // and reads the horizontal scroll offset.
    UIScrollView scroller = createHorizontalScroller(false);
    scroller.getView().setScrollTo(130, 0, false);
    scroller.setItemSnap(createSnapParams(0, 1));

    ScrollSnapHelper helper = getScrollSnapHelper(scroller.getView());
    SnapTarget target = helper.findSnapTarget(0);

    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);
  }

  @Test
  public void testHorizontalRtlSnapNormalizesOffsetAndChildStart() throws Exception {
    // Verify that horizontal RTL converts Android's physical scrollX and right-to-left child.left
    // values into increasing logical coordinates.
    UIScrollView scroller = createHorizontalScroller(true);
    scroller.getView().setScrollTo(200, 0, false);
    scroller.setItemSnap(createSnapParams(0, 1));

    ScrollSnapHelper helper = getScrollSnapHelper(scroller.getView());
    ScrollContainerHooks hooks = getScrollContainerHooks(helper);
    SnapTarget target = helper.findSnapTarget(0);

    assertEquals(100, hooks.getCurrentOffset());
    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);
  }

  @Test
  public void testInvalidItemSnapConfigurationStillInstallsFallbackHelper() throws Exception {
    // Verify that hooks report invalid factor and maxSnapCount values while the scroll-view still
    // installs a helper with fallback behavior.
    UIScrollView scroller = createVerticalScroller();
    JavaOnlyMap params = createSnapParams(-1, 0);
    scroller.getView().setScrollTo(0, 50, false);

    scroller.setItemSnap(params);

    ScrollSnapHelper helper = getScrollSnapHelper(scroller.getView());
    assertNotNull(helper);
    assertEquals(1, helper.findSnapTarget(10000).index);
  }

  @Test
  public void testDestroyClearsScrollSnapHelper() throws Exception {
    // Verify that destroying UIScrollView releases AndroidScrollView's references to the helper
    // and UI hooks.
    UIScrollView scroller = createVerticalScroller();
    AndroidScrollView view = scroller.getView();
    scroller.setItemSnap(createSnapParams(0, 1));
    assertNotNull(getScrollSnapHelper(view));

    scroller.destroy();

    assertNull(getScrollSnapHelper(view));
  }

  @Test
  public void testSnapTargetDispatchesSnapEvent() throws Exception {
    // Verify that selecting a snap target dispatches the same event details as UIListContainer.
    EventEmitter eventEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(eventEmitter);
    UIScrollView scroller = createVerticalScroller();
    Map<String, EventsListener> events = new HashMap<>();
    events.put("snap", new EventsListener("snap", "bindEvent", "onSnap", null, null));
    scroller.setEvents(events);
    scroller.getView().setScrollTo(0, 130, false);
    scroller.setItemSnap(createSnapParams(0, 1));

    assertTrue(invokeSnapToTarget(scroller.getView(), 0));

    ArgumentCaptor<LynxCustomEvent> eventCaptor = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(eventEmitter).sendCustomEvent(eventCaptor.capture());
    LynxCustomEvent event = eventCaptor.getValue();
    assertEquals("snap", event.getName());
    assertEquals(1, event.getTag());
    assertEquals("detail", event.paramsName());
    assertEquals(1, event.eventParams().get("position"));
    assertEquals(0f, getEventOffset(event, "currentScrollLeft"), 0f);
    assertEquals(PixelUtils.pxToDip(130), getEventOffset(event, "currentScrollTop"), 0f);
    assertEquals(0f, getEventOffset(event, "targetScrollLeft"), 0f);
    assertEquals(PixelUtils.pxToDip(100), getEventOffset(event, "targetScrollTop"), 0f);
  }

  private UIScrollView createVerticalScroller() {
    UIScrollView scroller = createScroller();
    scroller.setScrollY(true);
    scroller.getView().setMeasuredSize(VIEWPORT_SIZE, ITEM_SIZE * ITEM_COUNT);
    for (int index = 0; index < ITEM_COUNT; index++) {
      UIView child = createChild(index);
      child.setLeft(0);
      child.setTop(index * ITEM_SIZE);
      scroller.insertChild(child, index);
    }
    AndroidScrollView view = scroller.getView();
    view.layout(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    view.getHScrollView().layout(0, 0, VIEWPORT_SIZE, ITEM_SIZE * ITEM_COUNT);
    view.getLinearLayout().layout(0, 0, VIEWPORT_SIZE, ITEM_SIZE * ITEM_COUNT);
    return scroller;
  }

  private UIScrollView createHorizontalScroller(boolean rtl) {
    UIScrollView scroller = createScroller();
    scroller.setScrollX(true);
    scroller.getView().setMeasuredSize(ITEM_SIZE * ITEM_COUNT, VIEWPORT_SIZE);
    if (rtl) {
      scroller.setLynxDirection(DIRECTION_RTL);
    }
    for (int index = 0; index < ITEM_COUNT; index++) {
      UIView child = createChild(index);
      child.setLeft((rtl ? ITEM_COUNT - index - 1 : index) * ITEM_SIZE);
      child.setTop(0);
      scroller.insertChild(child, index);
    }
    AndroidScrollView view = scroller.getView();
    view.layout(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    view.getHScrollView().layout(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    view.getLinearLayout().layout(0, 0, ITEM_SIZE * ITEM_COUNT, VIEWPORT_SIZE);
    return scroller;
  }

  private UIScrollView createScroller() {
    UIScrollView scroller = new UIScrollView(mContext);
    scroller.setSign(1, "scroll-view");
    scroller.setWidth(VIEWPORT_SIZE);
    scroller.setHeight(VIEWPORT_SIZE);
    mUIOwner.setNode(scroller.getSign(), scroller);
    return scroller;
  }

  private UIView createChild(int index) {
    UIView child = new UIView(mContext);
    child.setSign(index + 2, "view");
    child.setWidth(ITEM_SIZE);
    child.setHeight(ITEM_SIZE);
    mUIOwner.setNode(child.getSign(), child);
    return child;
  }

  private JavaOnlyMap createSnapParams(double factor, int maxSnapCount) {
    JavaOnlyMap params = new JavaOnlyMap();
    params.putDouble("factor", factor);
    params.putInt("offset", 0);
    params.putInt("maxSnapCount", maxSnapCount);
    return params;
  }

  private ScrollSnapHelper getScrollSnapHelper(AndroidScrollView view) throws Exception {
    Field field = AndroidScrollView.class.getDeclaredField("mScrollSnapHelper");
    field.setAccessible(true);
    return (ScrollSnapHelper) field.get(view);
  }

  private ScrollContainerHooks getScrollContainerHooks(ScrollSnapHelper helper) throws Exception {
    Field field = ScrollSnapHelper.class.getDeclaredField("mHooks");
    field.setAccessible(true);
    return (ScrollContainerHooks) field.get(helper);
  }

  private boolean invokeSnapToTarget(AndroidScrollView view, int velocity) throws Exception {
    Method method = AndroidScrollView.class.getDeclaredMethod("snapToTarget", int.class);
    method.setAccessible(true);
    return (boolean) method.invoke(view, velocity);
  }

  private float getEventOffset(LynxCustomEvent event, String key) {
    return ((Number) event.eventParams().get(key)).floatValue();
  }
}
