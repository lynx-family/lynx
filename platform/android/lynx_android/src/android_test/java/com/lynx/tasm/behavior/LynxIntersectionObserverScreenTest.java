// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.RectF;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.utils.LynxConstants;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

public class LynxIntersectionObserverScreenTest {
  private final RectF mPageBounds = new RectF(600, 300, 1200, 1300);
  private final RectF mTargetBounds = new RectF(mPageBounds);
  private LynxContext mContext;
  private UIBody mRoot;
  private LynxBaseUI mTarget;
  private LynxIntersectionObserverManager mManager;
  private LynxIntersectionObserver mObserver;

  @Before
  public void setUp() {
    mContext = mock(LynxContext.class);
    LynxUIOwner owner = mock(LynxUIOwner.class);
    mRoot = mock(UIBody.class);
    mTarget = mock(LynxBaseUI.class);
    mManager = mock(LynxIntersectionObserverManager.class);
    when(mManager.getContext()).thenReturn(mContext);
    when(mContext.getLynxUIOwner()).thenReturn(owner);
    when(mContext.getUIBody()).thenReturn(mRoot);
    when(owner.getRootUI()).thenReturn(mRoot);
    when(mContext.findLynxUIBySign(1)).thenReturn(mTarget);
    when(mRoot.isVisible()).thenReturn(true);
    when(mTarget.isVisible()).thenReturn(true);
    when(mTarget.getParent()).thenReturn(mRoot);
    when(mManager.getBoundsOnScreenOfLynxBaseUI(mRoot))
        .thenAnswer(invocation -> new RectF(mPageBounds));
    when(mManager.getBoundsOnScreenOfLynxBaseUI(mTarget))
        .thenAnswer(invocation -> new RectF(mTargetBounds));
    when(mManager.getWindowRect(mContext)).thenAnswer(invocation -> new RectF(0, 0, 1200, 2670));
    mObserver = new LynxIntersectionObserver(
        mManager, 1, LynxConstants.LYNX_DEFAULT_COMPONENT_ID, new JavaOnlyMap());
  }

  @Test
  public void fullyVisibleRightColumnKeepsScreenCoordinates() {
    observeScreen();
    assertEntry(getCallback(), mTargetBounds, mTargetBounds, 1);
  }

  @Test
  public void fullyVisibleLowerCardKeepsScreenCoordinates() {
    setBounds(new RectF(0, 1500, 600, 2500));
    observeScreen();
    assertEntry(getCallback(), mTargetBounds, mTargetBounds, 1);
  }

  @Test
  public void bottomEdgeClipsOnlyIntersectionRect() {
    setBounds(new RectF(600, 2200, 1200, 3200));
    observeScreen();
    assertEntry(getCallback(), mTargetBounds, new RectF(600, 2200, 1200, 2670), 0.47);
  }

  @Test
  public void screenIntersectionPreservesAncestorClipping() {
    LynxBaseUI clippingParent = mock(LynxBaseUI.class);
    when(mTarget.getParent()).thenReturn(clippingParent);
    when(clippingParent.getParent()).thenReturn(mRoot);
    when(clippingParent.isVisible()).thenReturn(true);
    when(clippingParent.getOverflow()).thenReturn(0);
    when(mManager.getBoundsOnScreenOfLynxBaseUI(clippingParent))
        .thenAnswer(invocation -> new RectF(600, 300, 1200, 800));
    observeScreen();
    assertEntry(getCallback(), mTargetBounds, new RectF(600, 300, 1200, 800), 0.5);
  }

  @Test
  public void offscreenCardDoesNotSendInitialZeroRatio() {
    setBounds(new RectF(600, 2700, 1200, 3700));
    observeScreen();
    verify(mManager, never()).callIntersectionObserver(anyInt(), anyInt(), any(JavaOnlyMap.class));
  }

  @Test
  public void cardEnteringScreenAfterInitialZeroRatioSendsCallback() {
    setBounds(new RectF(600, 2700, 1200, 3700));
    observeScreen();
    verify(mManager, never()).callIntersectionObserver(anyInt(), anyInt(), any(JavaOnlyMap.class));
    setBounds(new RectF(600, 1500, 1200, 2500));
    mObserver.checkForIntersections();
    assertEntry(getCallback(), mTargetBounds, mTargetBounds, 1);
  }

  @Test
  public void viewportIntersectionRemainsRelativeToPage() {
    setBounds(new RectF(600, 2700, 1200, 3700));
    mObserver.relativeToViewport(new JavaOnlyMap());
    mObserver.observe("1", 7);
    assertEntry(getCallback(), mTargetBounds, mTargetBounds, 1);
  }

  @Test
  public void observingRootPreservesUnclippedBoundingClientRect() {
    setBounds(new RectF(600, 2200, 1200, 3200));
    when(mContext.findLynxUIBySign(1)).thenReturn(mRoot);
    observeScreen();
    assertEntry(getCallback(), mPageBounds, new RectF(600, 2200, 1200, 2670), 0.47);
  }

  @Test
  public void pageAtScreenOriginStillReportsFullIntersection() {
    setBounds(new RectF(0, 0, 600, 1000));
    observeScreen();
    assertEntry(getCallback(), mTargetBounds, mTargetBounds, 1);
  }

  private void setBounds(RectF bounds) {
    mPageBounds.set(bounds);
    mTargetBounds.set(bounds);
  }

  private void observeScreen() {
    mObserver.relativeToScreen(new JavaOnlyMap());
    mObserver.observe("1", 7);
  }

  private JavaOnlyMap getCallback() {
    ArgumentCaptor<JavaOnlyMap> callback = ArgumentCaptor.forClass(JavaOnlyMap.class);
    verify(mManager).callIntersectionObserver(eq(1), eq(7), callback.capture());
    return callback.getValue();
  }

  private static void assertEntry(
      JavaOnlyMap entry, RectF bounds, RectF intersection, double ratio) {
    assertTrue(entry.getBoolean("isIntersecting"));
    assertEquals(ratio, entry.getDouble("intersectionRatio"), 0.0001);
    assertRect(bounds, entry.getMap("boundingClientRect"));
    assertRect(intersection, entry.getMap("intersectionRect"));
  }

  private static void assertRect(RectF expected, ReadableMap actual) {
    assertEquals(expected.left, actual.getDouble("left"), 0.0001);
    assertEquals(expected.top, actual.getDouble("top"), 0.0001);
    assertEquals(expected.right, actual.getDouble("right"), 0.0001);
    assertEquals(expected.bottom, actual.getDouble("bottom"), 0.0001);
  }
}
