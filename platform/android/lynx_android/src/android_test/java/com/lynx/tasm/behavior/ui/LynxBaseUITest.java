// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.os.SystemClock;
import android.view.MotionEvent;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

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

  private void setEnableNativeInteraction(boolean enable) {
    JavaOnlyMap pageConfig = new JavaOnlyMap();
    pageConfig.putBoolean("enableNativeInteraction", enable);
    mContext.onPageConfigDecoded(new PageConfig(pageConfig));
  }

  private void setEnableEventThroughInheritFromPage(boolean enable) {
    JavaOnlyMap pageConfig = new JavaOnlyMap();
    pageConfig.putBoolean("enableEventThroughInheritFromPage", enable);
    mContext.onPageConfigDecoded(new PageConfig(pageConfig));
  }

  private void updateLayoutInfo(LynxBaseUI ui, int left, int top, int width, int height) {
    ui.updateLayoutInfo(left, top, width, height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, null);
  }

  private void assertTapAfterLayoutUpdate(
      boolean flatten, boolean moveParent, int left, int top, boolean expectTap) {
    UIBody root = TestingUtils.getUIBody(mContext);
    root.setSign(1, "page");
    updateLayoutInfo(root, 0, 0, 500, 500);
    LynxBaseUI parent = flatten ? new LynxFlattenUI(mContext) : new UIView(mContext);
    parent.setSign(2, "view");
    updateLayoutInfo(parent, 100, 200, 300, 300);
    UIView child = new UIView(mContext);
    child.setSign(3, "view");
    updateLayoutInfo(child, 10, 20, 100, 100);
    root.insertChild(parent, 0);
    parent.insertChild(child, 0);
    if (flatten) {
      ((LynxFlattenUI) parent).layout(100, 200, null);
      assertEquals(110, child.getLeft());
      assertEquals(220, child.getTop());
    }

    EventEmitter emitter = mock(EventEmitter.class);
    mContext.setEventEmitter(emitter);
    LynxUIOwner owner = mock(LynxUIOwner.class);
    when(owner.getContext()).thenReturn(mContext);
    when(owner.getRootUI()).thenReturn(root);
    TouchEventDispatcher dispatcher = new TouchEventDispatcher(owner);
    mContext.setTouchEventDispatcher(dispatcher);
    long downTime = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, 120, 230, 0);
    MotionEvent move =
        MotionEvent.obtain(downTime, downTime + 10, MotionEvent.ACTION_MOVE, 121, 231, 0);
    MotionEvent up =
        MotionEvent.obtain(downTime, downTime + 20, MotionEvent.ACTION_UP, 121, 231, 0);
    try {
      assertTrue(dispatcher.handleFirstTouchDown(down, root));
      dispatcher.handleTouchMove(move);
      assertTrue(dispatcher.isTouchMoving());
      if (moveParent) {
        updateLayoutInfo(parent, left, top, 300, 300);
      } else {
        updateLayoutInfo(child, left, top, 100, 100);
      }
      dispatcher.fireTap(up);

      ArgumentCaptor<LynxTouchEvent> events = ArgumentCaptor.forClass(LynxTouchEvent.class);
      verify(emitter, atLeastOnce()).sendTouchEvent(events.capture());
      int tapCount = 0;
      for (LynxTouchEvent event : events.getAllValues()) {
        if ("tap".equals(event.getName())) {
          assertEquals(child.getSign(), event.getTag());
          tapCount++;
        }
      }
      assertEquals(expectTap ? 1 : 0, tapCount);
    } finally {
      down.recycle();
      move.recycle();
      up.recycle();
      mContext.setTouchEventDispatcher(null);
    }
  }

  @Test
  public void tapSurvivesUnchangedLayoutUnderFlattenParent() {
    assertTapAfterLayoutUpdate(true, false, 10, 20, true);
  }

  @Test
  public void tapSurvivesUnchangedLayoutWithoutFlattenParent() {
    assertTapAfterLayoutUpdate(false, false, 10, 20, true);
  }

  @Test
  public void tapIsSuppressedWhenChildMovesToPreviousDrawingLeft() {
    assertTapAfterLayoutUpdate(true, false, 110, 20, false);
  }

  @Test
  public void tapIsSuppressedWhenChildMovesToPreviousDrawingTop() {
    assertTapAfterLayoutUpdate(true, false, 10, 220, false);
  }

  @Test
  public void tapIsSuppressedWhenChildMovesToPreviousDrawingPosition() {
    assertTapAfterLayoutUpdate(true, false, 110, 220, false);
  }

  @Test
  public void tapIsSuppressedWhenFlattenParentMoves() {
    assertTapAfterLayoutUpdate(true, true, 110, 200, false);
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
  public void displayNoneAffectsVisibility() {
    LynxBaseUI ui = new UIView(mContext);
    assertTrue(ui.getVisibility());

    ui.setDisplayNone(true);
    assertFalse(ui.getVisibility());

    ui.setDisplayNone(false);
    assertTrue(ui.getVisibility());
  }

  @Test
  public void memoryUsageIncludesBackgroundImages() {
    LynxBaseUI ui = new UIView(mContext);
    LynxBackground background = mock(LynxBackground.class);
    BackgroundDrawable drawable = mock(BackgroundDrawable.class);
    when(background.getDrawable()).thenReturn(drawable);
    when(drawable.getMemoryUsageBytes()).thenReturn(300L);
    ui.setLynxBackground(background);

    assertEquals(300, ui.getMemoryUsageBytes());
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

  @Test
  public void nativeInteractionDefaultsToPageConfigWhenPropUnset() {
    setEnableNativeInteraction(true);

    UIView ui = new UIView(mContext);

    assertTrue(ui.nativeInteractionEnabled);
  }

  @Test
  public void eventThroughInheritsFromPageOnlyWhenEnabled() {
    UIBody root = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    UIView child = new UIView(mContext);
    root.insertChild(child, 0);
    root.mEventThrough = EventTarget.EnableStatus.Enable;

    assertFalse(child.eventThrough(0.f, 0.f));

    setEnableEventThroughInheritFromPage(true);
    assertTrue(child.eventThrough(0.f, 0.f));

    child.mEventThrough = EventTarget.EnableStatus.Disable;
    assertFalse(child.eventThrough(0.f, 0.f));
  }
}
