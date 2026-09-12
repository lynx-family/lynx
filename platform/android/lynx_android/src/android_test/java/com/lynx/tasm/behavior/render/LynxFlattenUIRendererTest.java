// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LynxFlattenUIRendererTest {
  @Mock private Canvas mockCanvas;

  private LynxContext lynxContext;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    lynxContext = TestingUtils.getLynxContext();
  }

  @Test
  public void testFlattenUIIsNotRendererHost() {
    assertFalse(new LynxFlattenUI(lynxContext) instanceof IRendererHost);
  }

  @Test
  public void testLegacyDrawingKeepsPropertiesOffDrawParent() {
    UIView drawParent = new UIView(lynxContext);
    View drawParentView = drawParent.getView();
    LynxFlattenUI flattenUI = spy(new LynxFlattenUI(lynxContext));
    flattenUI.setDrawParent(drawParent);
    flattenUI.setLeft(10);
    flattenUI.setTop(15);
    flattenUI.setWidth(100);
    flattenUI.setHeight(50);
    flattenUI.setAlpha(0.5f);
    doNothing().when(flattenUI).onDraw(mockCanvas);
    when(mockCanvas.save()).thenReturn(1);

    flattenUI.draw(mockCanvas);

    verify(mockCanvas).translate(10f, 15f);
    verify(mockCanvas).saveLayerAlpha(0f, 0f, 100f, 50f, 127, Canvas.ALL_SAVE_FLAG);
    verify(flattenUI).onDraw(mockCanvas);
    verify(mockCanvas).restoreToCount(1);
    assertEquals(1f, drawParentView.getAlpha(), 0.0001f);
    assertEquals(0f, drawParentView.getTranslationX(), 0.0001f);
    assertEquals(0f, drawParentView.getTranslationY(), 0.0001f);
  }
}
