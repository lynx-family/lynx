// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.graphics.Rect;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.IListNodeInfoFetcher;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.view.ComponentView;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ListItemTransformerTest {
  private LynxContext context;
  private UIListContainer list;
  private ListContainerView view;

  private static class RecordingTransformer implements ListItemTransformer {
    final List<String> events;
    final String name;
    int transforms;
    int resets;
    int offset;
    boolean vertical;
    boolean rtl;
    float scale = 0.75f;

    RecordingTransformer(String name, List<String> events) {
      this.name = name;
      this.events = events;
    }

    RecordingTransformer() {
      this("transformer", new ArrayList<>());
    }

    @Override
    public void transformItem(@NonNull ListContainerView container, @NonNull View item,
        boolean isVertical, boolean isRTL, int mainAxisOffset) {
      transforms++;
      offset = mainAxisOffset;
      vertical = isVertical;
      rtl = isRTL;
      events.add(name + ":transform");
      item.setScaleX(scale);
    }

    @Override
    public void resetItem(@NonNull View item) {
      resets++;
      events.add(name + ":reset");
      item.setScaleX(1);
    }
  }

  @Before
  public void setUp() {
    context = TestingUtils.getLynxContext();
    context.setListNodeInfoFetcher(mock(IListNodeInfoFetcher.class));
    context.setEventEmitter(mock(EventEmitter.class));
    list = new UIListContainer(context);
    list.setScrollOrientation("vertical");
    view = list.getView();
    view.layout(0, 0, 300, 300);
    view.getLinearLayout().layout(0, 0, 1200, 1200);
  }

  private View addItem(int left, int top) {
    View item = new View(context);
    item.layout(left, top, left + 100, top + 100);
    view.addView(item);
    return item;
  }

  @Test
  public void noTransformerPreservesExistingVisualProperties() {
    // Without a transformer, layout, scrolling, explicit refresh, and removal preserve existing
    // visual properties.
    View item = addItem(20, 40);
    item.setScaleX(0.6f);
    item.setTranslationY(12);
    item.layout(20, 60, 120, 160);
    view.setScrollY(10);
    list.requestListItemTransform();
    assertEquals(60, item.getTop());
    assertEquals(0.6f, item.getScaleX(), 0.001f);
    view.removeView(item);
    assertEquals(0.6f, item.getScaleX(), 0.001f);
    assertEquals(12, item.getTranslationY(), 0.001f);
  }

  @Test
  public void installingTransformerCoversExistingAndNewItems() {
    // Installing a transformer updates existing items immediately; newly attached items are
    // transformed as soon as they are added.
    View existing = addItem(0, 40);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    assertEquals(1, transformer.transforms);
    assertEquals(0.75f, existing.getScaleX(), 0.001f);
    View added = addItem(0, 160);
    assertEquals(2, transformer.transforms);
    assertEquals(160, transformer.offset);
    assertEquals(0.75f, added.getScaleX(), 0.001f);
  }

  @Test
  public void itemLayoutAndScrollUseLatestPhysicalFrame() {
    // Refresh after item layout changes, then use the updated top minus the latest scrollY.
    View item = addItem(0, 40);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    item.layout(0, 80, 100, 180);
    assertEquals(2, transformer.transforms);
    assertEquals(80, transformer.offset);
    view.setScrollY(30);
    assertTrue(transformer.transforms > 2);
    assertEquals(50, transformer.offset);
    assertEquals(80, item.getTop());
  }

  @Test
  public void horizontalRtlUsesRightEdgesAndRefreshesOnViewportResize() {
    // Horizontal RTL uses right edges. Resizing only the viewport must recalculate the offset.
    list.setScrollOrientation("horizontal");
    list.setLynxDirection(StyleConstants.DIRECTION_RTL);
    View item = addItem(150, 0);
    view.setScrollX(20);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    assertTrue(transformer.rtl);
    assertFalse(transformer.vertical);
    assertEquals(70, transformer.offset);
    view.layout(0, 0, 400, 300);
    assertEquals(view.getScrollX() + 400 - item.getRight(), transformer.offset);
    assertEquals(150, item.getLeft());
  }

  @Test
  public void horizontalLtrAndVerticalRtlUseTheirOwnMainAxis() {
    // Switching between horizontal LTR and vertical RTL uses left - scrollX and top - scrollY.
    list.setScrollOrientation("horizontal");
    addItem(150, 80);
    view.setScrollX(20);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    assertFalse(transformer.rtl);
    assertEquals(130, transformer.offset);
    list.setScrollOrientation("vertical");
    list.setLynxDirection(StyleConstants.DIRECTION_RTL);
    view.setScrollY(30);
    list.requestListItemTransform();
    assertTrue(transformer.vertical);
    assertTrue(transformer.rtl);
    assertEquals(50, transformer.offset);
  }

  @Test
  public void sameInstanceNeedsExplicitRefreshAndDoesNotDuplicateListeners() {
    // Setting the same instance does not reset or refresh. Explicit refresh applies changed
    // parameters, and each subsequent layout triggers exactly one callback.
    View item = addItem(0, 0);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    transformer.scale = 0.5f;
    list.setListItemTransformer(transformer);
    list.setListItemTransformer(transformer);
    assertEquals(1, transformer.transforms);
    assertEquals(0, transformer.resets);
    list.requestListItemTransform();
    assertEquals(0.5f, item.getScaleX(), 0.001f);
    item.layout(0, 10, 100, 110);
    assertEquals(3, transformer.transforms);
  }

  @Test
  public void replacementResetsOldTransformerBeforeApplyingNewOne() {
    // Reset items through the old transformer before applying the new one so stale state cannot
    // overwrite the new result.
    addItem(0, 0);
    List<String> events = new ArrayList<>();
    RecordingTransformer oldTransformer = new RecordingTransformer("old", events);
    RecordingTransformer newTransformer = new RecordingTransformer("new", events);
    list.setListItemTransformer(oldTransformer);
    events.clear();
    list.setListItemTransformer(newTransformer);
    assertEquals(Arrays.asList("old:reset", "new:transform"), events);
  }

  @Test
  public void clearStopsCallbacksAndReinstallRestoresListeners() {
    // Clearing stops layout callbacks for existing and new items; reinstalling restores listeners
    // for both.
    View existing = addItem(0, 0);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    list.setListItemTransformer(null);
    assertEquals(1, transformer.resets);
    assertEquals(1, existing.getScaleX(), 0.001f);
    existing.layout(0, 10, 100, 110);
    View added = addItem(0, 120);
    list.requestListItemTransform();
    assertEquals(1, transformer.transforms);
    list.setListItemTransformer(transformer);
    int calls = transformer.transforms;
    existing.layout(0, 20, 100, 120);
    added.layout(0, 130, 100, 230);
    assertEquals(calls + 2, transformer.transforms);
  }

  @Test
  public void detachResetsItemAndReuseRegistersOneListener() {
    // Removal resets visual properties and stops layout callbacks. Reusing the same View restores
    // callbacks without duplicate listeners.
    View item = addItem(0, 0);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    view.removeView(item);
    assertEquals(1, transformer.resets);
    assertEquals(1, item.getScaleX(), 0.001f);
    item.layout(0, 20, 100, 120);
    assertEquals(1, transformer.transforms);
    view.addView(item);
    item.layout(0, 30, 100, 130);
    assertEquals(3, transformer.transforms);
  }

  @Test
  public void stickyAndTransformerAreMutuallyExclusive() {
    // Reject transformers while sticky is enabled; enabling sticky clears and resets an installed
    // transformer.
    View item = addItem(0, 0);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setEnableListSticky(true);
    list.setListItemTransformer(transformer);
    assertFalse(view.hasListItemTransformer());
    assertEquals(0, transformer.transforms);
    list.setEnableListSticky(false);
    list.setListItemTransformer(transformer);
    list.setEnableListSticky(true);
    assertFalse(view.hasListItemTransformer());
    assertEquals(1, transformer.resets);
    assertEquals(1, item.getScaleX(), 0.001f);
  }

  @Test
  public void destroyResetsItemsAndRemovesLayoutListeners() {
    // Destruction resets attached items. Later layout changes on externally retained items must
    // not invoke the old transformer.
    View item = addItem(0, 0);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    view.destroy();
    assertEquals(1, transformer.resets);
    assertEquals(1, item.getScaleX(), 0.001f);
    item.layout(0, 20, 100, 120);
    assertEquals(1, transformer.transforms);
  }

  @Test
  public void carryMirrorsHorizontalTranslationForRtlAndPreservesFrame() {
    // At half an item, scale is 0.9. RTL reverses horizontal translation without changing layout;
    // reset clears the transformer's effects.
    View item = addItem(20, 40);
    CarryListItemTransformer transformer = new CarryListItemTransformer();
    transformer.setNormTranslationFactor(1);
    transformer.transformItem(view, item, false, false, 50);
    assertEquals(0.9f, item.getScaleX(), 0.001f);
    assertEquals(-5, item.getTranslationX(), 0.001f);
    transformer.transformItem(view, item, false, true, 50);
    assertEquals(5, item.getTranslationX(), 0.001f);
    assertEquals(20, item.getLeft());
    assertEquals(40, item.getTop());
    transformer.resetItem(item);
    assertEquals(1, item.getScaleX(), 0.001f);
    assertEquals(0, item.getTranslationX(), 0.001f);
  }

  @Test
  public void carryClampsScaleAndPreservesTranslationItDoesNotOwn() {
    // With Carry translation disabled, preserve translation from other sources. Clamp scale to
    // its minimum at large distances, and leave external translation unchanged on reset.
    View item = addItem(0, 0);
    item.setTranslationY(12);
    CarryListItemTransformer transformer = new CarryListItemTransformer();
    transformer.transformItem(view, item, true, false, -1000);
    assertEquals(0.8f, item.getScaleY(), 0.001f);
    assertEquals(12, item.getTranslationY(), 0.001f);
    transformer.resetItem(item);
    assertEquals(1, item.getScaleY(), 0.001f);
    assertEquals(12, item.getTranslationY(), 0.001f);
  }

  @Test
  public void rendererLayoutPreservesFrameAndRefreshesTransformer() {
    // Apply renderer frames directly. Preserve visual properties without a transformer, then use
    // the actual layout offset after a transformer is installed.
    ComponentView item = new ComponentView(context);
    Renderer itemRenderer = mock(Renderer.class);
    when(itemRenderer.getLynxFrame()).thenReturn(new Rect(0, 80, 100, 180));
    item.setRenderer(itemRenderer);
    view.addView(item);
    view.setRenderer(mock(Renderer.class));
    item.setScaleX(0.6f);
    view.onLayout(false, 0, 0, 300, 300);
    assertEquals(80, item.getTop());
    assertEquals(0.6f, item.getScaleX(), 0.001f);
    RecordingTransformer transformer = new RecordingTransformer();
    list.setListItemTransformer(transformer);
    when(itemRenderer.getLynxFrame()).thenReturn(new Rect(0, 120, 100, 220));
    view.onLayout(false, 0, 0, 300, 300);
    assertEquals(120, item.getTop());
    assertEquals(120 - view.getScrollY(), transformer.offset);
    assertTrue(transformer.transforms > 1);
  }

  @Test
  public void carryFallsBackToViewportAndResetsInvalidConfiguration() {
    // Use the viewport size before item layout. Invalid scale ranges reset the scale and
    // translation previously applied by Carry.
    View item = new View(context);
    CarryListItemTransformer transformer = new CarryListItemTransformer();
    transformer.setNormTranslationFactor(1);
    transformer.transformItem(view, item, true, false, 150);
    assertEquals(0.9f, item.getScaleY(), 0.001f);
    assertEquals(-15, item.getTranslationY(), 0.001f);
    transformer.setMinScaleY(2);
    transformer.transformItem(view, item, true, false, 150);
    assertEquals(1, item.getScaleY(), 0.001f);
    assertEquals(0, item.getTranslationY(), 0.001f);
  }
}
