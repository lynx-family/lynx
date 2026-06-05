// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.os.Build;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LynxFlattenUIRendererTest {
  private static final int TEST_SIGN = 123;

  @Mock private PlatformRendererContext mockPlatformRendererContext;
  @Mock private Canvas mockCanvas;

  private LynxContext lynxContext;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    lynxContext = TestingUtils.getLynxContext();
  }

  @Test
  public void testSubtreePropertiesStayOnFlattenRendererHost() {
    UIView drawParent = new UIView(lynxContext);
    View drawParentView = drawParent.getView();
    LynxFlattenUI flattenUI = createFlattenRendererHost(drawParent);

    Renderer renderer = flattenUI.getRenderer();
    renderer.applySubtreeProperties(createOpacityBuffer(0.5f), 1);
    renderer.applySubtreeProperties(createTransformBuffer(20f, 30f), 1);

    assertSame(drawParentView, flattenUI.getView());
    assertEquals(1f, drawParentView.getAlpha(), 0.0001f);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      Matrix parentMatrix = drawParentView.getAnimationMatrix();
      assertTrue(parentMatrix == null || parentMatrix.isIdentity());
    } else {
      assertEquals(0f, drawParentView.getTranslationX(), 0.0001f);
      assertEquals(0f, drawParentView.getTranslationY(), 0.0001f);
      assertEquals(1f, drawParentView.getScaleX(), 0.0001f);
      assertEquals(1f, drawParentView.getScaleY(), 0.0001f);
    }

    flattenUI.setWidth(100);
    flattenUI.setHeight(50);
    when(mockCanvas.save()).thenReturn(1);
    flattenUI.draw(mockCanvas);

    verify(mockCanvas, atLeastOnce()).concat(any(Matrix.class));
    verify(mockCanvas, times(1)).saveLayerAlpha(0f, 0f, 100f, 50f, 127, Canvas.ALL_SAVE_FLAG);
  }

  @Test
  public void testIdentityTransformClearsFlattenRendererTransform() {
    UIView drawParent = new UIView(lynxContext);
    LynxFlattenUI flattenUI = createFlattenRendererHost(drawParent);

    Renderer renderer = flattenUI.getRenderer();
    renderer.applySubtreeProperties(createTransformBuffer(20f, 30f), 1);
    renderer.applySubtreeProperties(createTransformBuffer(0f, 0f), 1);

    flattenUI.setWidth(100);
    flattenUI.setHeight(50);
    flattenUI.draw(mockCanvas);

    verify(mockCanvas, org.mockito.Mockito.never()).concat(any(Matrix.class));
  }

  @Test
  public void testConvertPointInRendererHostToScreenUsesFlattenPositionAndRendererTransform() {
    UIView drawParent = new UIView(lynxContext);
    drawParent.getView().setScrollX(3);
    drawParent.getView().setScrollY(4);
    LynxFlattenUI flattenUI = createFlattenRendererHost(drawParent);
    flattenUI.setLeft(10);
    flattenUI.setTop(15);
    flattenUI.getRenderer().applySubtreeProperties(createTransformBuffer(20f, 30f), 1);

    PointF point = flattenUI.convertPointInRendererHostToScreen(new PointF(5, 6));

    assertEquals(32f, point.x, 0.0001f);
    assertEquals(47f, point.y, 0.0001f);
  }

  @Test
  public void testConvertPointInRendererHostToScreenWithoutDrawParentReturnsFallback() {
    LynxFlattenUI flattenUI = new LynxFlattenUI(lynxContext);
    Renderer renderer = flattenUI.createRenderer(mockPlatformRendererContext, TEST_SIGN);
    renderer.setRenderHost(flattenUI);
    flattenUI.setRenderer(renderer);
    renderer.applySubtreeProperties(createTransformBuffer(20f, 30f), 1);

    PointF point = flattenUI.convertPointInRendererHostToScreen(new PointF(5, 6));

    assertEquals(25f, point.x, 0.0001f);
    assertEquals(36f, point.y, 0.0001f);
  }

  private LynxFlattenUI createFlattenRendererHost(UIView drawParent) {
    LynxFlattenUI flattenUI = new LynxFlattenUI(lynxContext);
    flattenUI.setDrawParent(drawParent);
    Renderer renderer = flattenUI.createRenderer(mockPlatformRendererContext, TEST_SIGN);
    renderer.setRenderHost(flattenUI);
    flattenUI.setRenderer(renderer);
    assertNotNull(flattenUI.getRenderer());
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      assertNull(drawParent.getView().getAnimationMatrix());
    }
    return flattenUI;
  }

  private ByteBuffer createOpacityBuffer(float opacity) {
    ByteBuffer buffer = ByteBuffer.allocate(68).order(ByteOrder.nativeOrder());
    buffer.putInt(DisplayListApplier.SUBTREE_OP_OPACITY);
    buffer.putFloat(opacity);
    buffer.position(0);
    return buffer;
  }

  private ByteBuffer createTransformBuffer(float translateX, float translateY) {
    ByteBuffer buffer = ByteBuffer.allocate(68).order(ByteOrder.nativeOrder());
    buffer.putInt(DisplayListApplier.SUBTREE_OP_TRANSFORM);
    float[] transform = new float[16];
    transform[0] = 1f;
    transform[5] = 1f;
    transform[10] = 1f;
    transform[12] = translateX;
    transform[13] = translateY;
    transform[15] = 1f;
    for (float value : transform) {
      buffer.putFloat(value);
    }
    buffer.position(0);
    return buffer;
  }
}
