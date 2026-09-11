// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.view.View;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

public class AndroidViewMeasureLayoutTest {
  @Mock private Renderer mRenderer;
  @Mock private IDrawChildHook mDrawChildHook;

  private TestAndroidView mView;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    mView = new TestAndroidView();
    mView.measure(exactly(1), exactly(1));
    mView.layout(0, 0, 1, 1);
    mView.setRenderer(mRenderer);
    mView.bindDrawChildHook(mDrawChildHook);
  }

  @Test
  public void measureAndLayoutUseDrawChildHookInsteadOfRenderer() {
    mView.callOnMeasure(exactly(120), exactly(80));
    mView.callOnLayout(0, 0, 120, 80);

    assertEquals(120, mView.getMeasuredWidth());
    assertEquals(80, mView.getMeasuredHeight());
    verify(mDrawChildHook).performMeasureChildrenUI();
    verify(mDrawChildHook).performLayoutChildrenUI();
    verify(mRenderer, never()).getLynxFrame();
    verify(mRenderer, never()).onMeasure(exactly(120), exactly(80));
    verify(mRenderer, never()).onLayout(false, 0, 0, 120, 80);
  }

  private static int exactly(int size) {
    return View.MeasureSpec.makeMeasureSpec(size, View.MeasureSpec.EXACTLY);
  }

  private static class TestAndroidView extends AndroidView {
    TestAndroidView() {
      super(TestingUtils.getLynxContext());
    }

    void callOnMeasure(int widthMeasureSpec, int heightMeasureSpec) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    void callOnLayout(int left, int top, int right, int bottom) {
      super.onLayout(false, left, top, right, bottom);
    }
  }
}
