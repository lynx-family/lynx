// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.os.IBinder;
import android.view.View;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

public class LynxUITest {
  private LynxUI mUi;
  private LynxContext mContext;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUi = mock(LynxUI.class);
  }

  @After
  public void tearDown() throws Exception {
    mContext = null;
    mUi = null;
  }

  @Test
  public void testIsVisible() {
    doCallRealMethod().when(mUi).isVisible();
    View view = mock(View.class);
    // mView is null
    assertFalse(mUi.isVisible());
    mUi.mView = view;
    when(view.getVisibility())
        .thenReturn(View.INVISIBLE)
        .thenReturn(View.GONE)
        .thenReturn(View.VISIBLE);
    assertFalse(mUi.isVisible());
    assertFalse(mUi.isVisible());
    when(view.getAlpha()).thenReturn(0f).thenReturn(1f);
    assertFalse(mUi.isVisible());
    when(view.isAttachedToWindow()).thenReturn(false).thenReturn(true);
    assertFalse(mUi.isVisible());
    assertTrue(mUi.isVisible());
    // check getWindowToken
    when(view.isAttachedToWindow()).thenAnswer(new Answer<Boolean>() {
      @Override
      public Boolean answer(InvocationOnMock invocation) throws Throwable {
        return view.getWindowToken() != null;
      }
    });
    assertFalse(mUi.isVisible());
    when(view.getWindowToken()).thenReturn(mock(IBinder.class));
    assertTrue(mUi.isVisible());
  }

  private LynxUI createSpyLynxUI() {
    LynxUI realUI = new LynxUI(mContext) {};
    LynxUI ui = spy(realUI);
    ViewInfo info = mock(ViewInfo.class);
    ui.mViewInfo = info;

    View mockView = mock(View.class);
    ui.mView = mockView;
    return ui;
  }

  @Test
  public void testProcessViewInfo() {
    LynxUI ui = createSpyLynxUI();

    when(ui.getSkewX()).thenReturn(1.0f);
    when(ui.getSkewY()).thenReturn(2.0f);
    when(ui.getWidth()).thenReturn(3);
    when(ui.getHeight()).thenReturn(4);

    ui.processViewInfo();

    verify(ui, times(1)).beforeProcessViewInfo(ui.mViewInfo);
    verify(ui, times(1)).beforeDispatchProcessViewInfo(ui.mViewInfo);
    verify(ui, times(1)).dispatchProcessViewInfo();
    verify(ui, times(1)).afterDispatchProcessViewInfo(ui.mViewInfo);
    verify(ui, times(1)).afterProcessViewInfo(ui.mViewInfo);

    verify(ui.mViewInfo, times(1)).setSkewX(1.0f);
    verify(ui.mViewInfo, times(1)).setSkewY(2.0f);
    verify(ui.mViewInfo, times(1)).setClipPath(null);
    verify(ui.mViewInfo, times(1)).setWidth(3);
    verify(ui.mViewInfo, times(1)).setHeight(4);
  }

  @Test
  public void testDispatchProcessViewInfo() {
    LynxUI parentUI = createSpyLynxUI();

    when(parentUI.getSkewX()).thenReturn(1.0f);
    when(parentUI.getSkewY()).thenReturn(2.0f);
    when(parentUI.getWidth()).thenReturn(3);
    when(parentUI.getHeight()).thenReturn(4);

    LynxUI childUI0 = createSpyLynxUI();
    LynxUI childUI1 = createSpyLynxUI();
    LynxUI childUI2 = createSpyLynxUI();

    parentUI.mChildren.add(childUI0);
    parentUI.mChildren.add(childUI1);
    parentUI.mChildren.add(childUI2);
    parentUI.mDrawHead = childUI0;
    childUI0.mNextDrawUI = childUI1;
    childUI1.mNextDrawUI = childUI2;

    parentUI.dispatchProcessViewInfo();

    verify(parentUI, times(1)).processChildViewInfo(childUI0);
    verify(parentUI, times(1))
        .beforeProcessChildViewInfo(parentUI.mViewInfo, childUI0.getView(), 0);
    verify(parentUI, times(1)).afterProcessChildViewInfo(parentUI.mViewInfo, childUI0.getView(), 0);

    verify(parentUI, times(1)).processChildViewInfo(childUI1);
    verify(parentUI, times(1))
        .beforeProcessChildViewInfo(parentUI.mViewInfo, childUI1.getView(), 0);
    verify(parentUI, times(1)).afterProcessChildViewInfo(parentUI.mViewInfo, childUI1.getView(), 0);

    verify(parentUI, times(1)).processChildViewInfo(childUI2);
    verify(parentUI, times(1))
        .beforeProcessChildViewInfo(parentUI.mViewInfo, childUI2.getView(), 0);
    verify(parentUI, times(1)).afterProcessChildViewInfo(parentUI.mViewInfo, childUI2.getView(), 0);
  }

  @Test
  public void testGetOrCreateView() {
    UIView ui = new UIView(mContext);
    assertNotNull(ui.getView());

    mContext.markFallbackProcess(true);
    mContext.setUIBodyView(new UIBody.UIBodyView(mContext));
    View view = new AndroidView(mContext);
    mContext.getUIBodyView().registerViewAccordingToNodeIndex(1, view);
    UIView fallbackUI = new UIView(mContext, new UIParams(1, 1, false, "", null, null, null));
    assertEquals(fallbackUI.getView(), view);
  }
}
