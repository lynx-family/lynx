// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.ListNodeInfoFetcher;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.list.container.ListContainerProxy;
import com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.ScrollContainerHooks;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.SnapItem;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.SnapTarget;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class UIListContainerTest {
  private LynxContext lynxContext;
  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
    LynxTemplateRender renderer = mock(LynxTemplateRender.class);
    lynxContext.setListNodeInfoFetcher(new ListNodeInfoFetcher(renderer));
    lynxContext.setEventEmitter(new LynxEventEmitter(renderer.getEngineProxy()));
    UIBody mUIBody = new UIBody(lynxContext, new UIBody.UIBodyView(lynxContext));
    LynxView mLynxView = new LynxView(lynxContext, new LynxViewBuilder());
    lynxContext.setLynxView(mLynxView);
  }
  @Test
  public void testSnap() throws Exception {
    UIListContainer uiList = new UIListContainer(lynxContext);
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("offset", 20);
    map.put("factor", 0);
    uiList.setPagingAlignment((ReadableMap) map);

    // Install ScrollSnapHelper by default when the switch is omitted, and convert offset once.
    ScrollSnapHelper scrollSnapHelper = getScrollSnapHelper(uiList);
    assertNotNull(scrollSnapHelper);
    assertNull(getLynxSnapHelper(uiList));
    assertEquals((int) PixelUtils.dipToPx(20), getAlignmentOffset(scrollSnapHelper));

    map.put("offset", 20);
    map.put("factor", -1);
    uiList.setPagingAlignment((ReadableMap) map);
    assertFalse(hasPendingScrollSnapHelperRtlWarning(uiList));

    map.put("factor", 0.5);
    uiList.setPagingAlignment((ReadableMap) map);
    assertTrue(hasPendingScrollSnapHelperRtlWarning(uiList));

    uiList.setPagingAlignment(null);
    assertNull(getScrollSnapHelper(uiList));
    assertNull(getLynxSnapHelper(uiList));
    assertFalse(hasPendingScrollSnapHelperRtlWarning(uiList));
  }

  @Test
  public void testSnapCanFallbackToLynxSnapHelper() throws Exception {
    UIListContainer uiList = new UIListContainer(lynxContext);
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("offset", 20);
    map.put("factor", 0);

    uiList.setPagingAlignment(map);

    // The switch defaults to true, so install the new implementation first.
    assertNull(getLynxSnapHelper(uiList));
    assertNotNull(getScrollSnapHelper(uiList));

    map.put("useScrollSnapHelper", false);

    uiList.setPagingAlignment(map);

    // Explicitly disabling the switch at runtime keeps only the legacy LynxSnapHelper and allows
    // a complete fallback.
    assertNotNull(getLynxSnapHelper(uiList));
    assertNull(getScrollSnapHelper(uiList));

    map.put("useScrollSnapHelper", true);
    uiList.setPagingAlignment(map);

    // Re-enabling the switch at runtime restores the new implementation and removes the legacy
    // helper so the two algorithms cannot run at the same time.
    assertNull(getLynxSnapHelper(uiList));
    assertNotNull(getScrollSnapHelper(uiList));
  }

  @Test
  public void testSnapMaxSnapCountProp() throws Exception {
    UIListContainer uiList = new UIListContainer(lynxContext);
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("offset", 0);
    map.put("factor", 0);
    map.put("maxSnapCount", 3);

    // Verify that item-snap.maxSnapCount is parsed and passed to ScrollSnapHelper.
    uiList.setPagingAlignment((ReadableMap) map);

    ScrollSnapHelper scrollSnapHelper = getScrollSnapHelper(uiList);
    assertNotNull(scrollSnapHelper);
    assertEquals(3, getMaxSnapCount(scrollSnapHelper));

    map.put("maxSnapCount", 0);

    // Verify that an invalid maxSnapCount falls back to 1 and preserves single-step snapping.
    uiList.setPagingAlignment((ReadableMap) map);

    scrollSnapHelper = getScrollSnapHelper(uiList);
    assertNotNull(scrollSnapHelper);
    assertEquals(1, getMaxSnapCount(scrollSnapHelper));
  }

  @Test
  public void testHorizontalRtlAdjacentSnapUsesDataIndexDirection() throws Exception {
    final int viewportSize = 100;
    final int itemSize = 100;
    final int itemCount = 3;
    final int contentSize = itemSize * itemCount;
    final int currentPhysicalOffset = 150;

    EventEmitter eventEmitter = mock(EventEmitter.class);
    lynxContext.setEventEmitter(eventEmitter);
    UIListContainer uiList = new UIListContainer(lynxContext);
    uiList.setScrollOrientation("horizontal");
    uiList.setLynxDirection(StyleConstants.DIRECTION_RTL);
    uiList.getView().layout(0, 0, viewportSize, viewportSize);
    uiList.getView().getChildAt(0).layout(0, 0, contentSize, viewportSize);
    setItemKeys(uiList, JavaOnlyArray.of("item-0", "item-1", "item-2"));
    for (int index = 0; index < itemCount; index++) {
      UIComponent item = new UIComponent(lynxContext);
      item.setItemKey("item-" + index);
      int physicalLeft = (itemCount - index - 1) * itemSize;
      item.setLeft(physicalLeft);
      item.setWidth(itemSize);
      item.setHeight(itemSize);
      item.getView().layout(physicalLeft, 0, physicalLeft + itemSize, itemSize);
      uiList.getView().addView(item.getView());
    }
    uiList.getView().setScrollX(currentPhysicalOffset);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("snap", new EventsListener("snap", "bindEvent", "onSnap", null, null));
    uiList.setEvents(events);

    JavaOnlyMap map = new JavaOnlyMap();
    map.put("offset", 0);
    map.put("factor", 0);
    uiList.setPagingAlignment(map);
    ScrollSnapHelper helper = getScrollSnapHelper(uiList);
    ScrollContainerHooks hooks = getScrollContainerHooks(helper);

    SnapItem rtlAdjacentItem = hooks.getAdjacentSnapItem(1, true);
    uiList.setLynxDirection(StyleConstants.DIRECTION_LTR);
    SnapItem ltrAdjacentItem = hooks.getAdjacentSnapItem(1, true);
    uiList.setLynxDirection(StyleConstants.DIRECTION_RTL);
    SnapTarget target = helper.findSnapTarget(1000);

    // ScrollSnapHelper uses normalized logical offsets, so forward moves toward the next data item
    // in both horizontal RTL and LTR lists.
    assertNotNull(rtlAdjacentItem);
    assertNotNull(ltrAdjacentItem);
    assertEquals(2, getSnapItemIndex(rtlAdjacentItem));
    assertEquals(2, getSnapItemIndex(ltrAdjacentItem));
    assertEquals(50, hooks.getCurrentOffset());
    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);

    // The helper target remains logical, while the event reports the physical Android scrollX.
    ArgumentCaptor<LynxCustomEvent> eventCaptor = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(eventEmitter).sendCustomEvent(eventCaptor.capture());
    LynxCustomEvent event = eventCaptor.getValue();
    assertEquals(1, event.eventParams().get("position"));
    assertEquals(
        PixelUtils.pxToDip(currentPhysicalOffset), getEventOffset(event, "currentScrollLeft"), 0f);
    assertEquals(PixelUtils.pxToDip(100), getEventOffset(event, "targetScrollLeft"), 0f);
  }

  @Test
  public void testContainerProxy() {
    UIListContainer uiList = new UIListContainer(lynxContext);
    ListContainerProxy listContainerProxy = mock(ListContainerProxy.class);
    listContainerProxy.scrollByListContainer(uiList.getSign(), 0, 0, 0, 0);
    listContainerProxy.scrollToPosition(uiList.getSign(), 0, 0, 0, false);
    listContainerProxy.scrollStopped(uiList.getSign());
  }

  private int getMaxSnapCount(ScrollSnapHelper snapHelper) throws Exception {
    Field field = ScrollSnapHelper.class.getDeclaredField("mMaxSnapCount");
    field.setAccessible(true);
    return field.getInt(snapHelper);
  }

  private int getAlignmentOffset(ScrollSnapHelper snapHelper) throws Exception {
    Field field = ScrollSnapHelper.class.getDeclaredField("mAlignmentOffset");
    field.setAccessible(true);
    return field.getInt(snapHelper);
  }

  private ScrollSnapHelper getScrollSnapHelper(UIListContainer uiList) throws Exception {
    Field field = NestedScrollContainerView.class.getDeclaredField("mScrollSnapHelper");
    field.setAccessible(true);
    return (ScrollSnapHelper) field.get(uiList.getView());
  }

  private LynxSnapHelper getLynxSnapHelper(UIListContainer uiList) throws Exception {
    Field field = NestedScrollContainerView.class.getDeclaredField("mLynxSnapHelper");
    field.setAccessible(true);
    return (LynxSnapHelper) field.get(uiList.getView());
  }

  private void setItemKeys(UIListContainer uiList, JavaOnlyArray itemKeys) throws Exception {
    Field field = UIListContainer.class.getDeclaredField("mItemKeys");
    field.setAccessible(true);
    field.set(uiList, itemKeys);
  }

  private boolean hasPendingScrollSnapHelperRtlWarning(UIListContainer uiList) throws Exception {
    Field field = UIListContainer.class.getDeclaredField("mPendingScrollSnapHelperRtlWarning");
    field.setAccessible(true);
    return field.getBoolean(uiList);
  }

  private ScrollContainerHooks getScrollContainerHooks(ScrollSnapHelper snapHelper)
      throws Exception {
    Field field = ScrollSnapHelper.class.getDeclaredField("mHooks");
    field.setAccessible(true);
    return (ScrollContainerHooks) field.get(snapHelper);
  }

  private int getSnapItemIndex(SnapItem snapItem) throws Exception {
    Field field = SnapItem.class.getDeclaredField("mIndex");
    field.setAccessible(true);
    return field.getInt(snapItem);
  }

  private float getEventOffset(LynxCustomEvent event, String key) {
    return ((Number) event.eventParams().get(key)).floatValue();
  }
}
