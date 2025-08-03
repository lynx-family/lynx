// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Path;
import android.graphics.Rect;
import com.lynx.tasm.behavior.ui.shapes.BasicShape;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.behavior.ui.utils.MaskDrawable;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.rendernode.compat.RenderNodeCompat;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class ViewInfoTest {
  @Before
  public void setUp() throws Exception {}

  @After
  public void tearDown() throws Exception {}

  @Test
  public void testSetDimensions() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);
    viewInfo.setWidth(100);
    viewInfo.setHeight(200);
    assertEquals(100, viewInfo.mWidth);
    assertEquals(200, viewInfo.mHeight);
  }

  @Test
  public void testClipPathApplication() {
    BasicShape mockShape = mock(BasicShape.class);
    when(mockShape.getPath(anyInt(), anyInt())).thenReturn(new Path());

    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);
    viewInfo.setClipPath(mockShape);
    viewInfo.beforeDraw(mockCanvas);

    verify(mockCanvas).clipPath(any(Path.class));
  }

  @Test
  public void testDrawingOrderWithHook() {
    int[] testOrder = {2, 1, 0};

    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);
    viewInfo.setDrawingOrder(testOrder);

    viewInfo.beforeDispatchDraw(mockCanvas);
    verify(mockHook).beforeDispatchProcessViewInfo(viewInfo);
  }

  @Test
  public void testSkewTransformation() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);
    viewInfo.setSkewX(0.5f);
    viewInfo.setSkewY(0.3f);
    viewInfo.beforeDraw(mockCanvas);

    verify(mockCanvas).skew(0.5f, 0.3f);
    verify(mockCanvas).translate(-anyFloat(), -anyFloat());
  }

  @Test
  public void testSubDrawInfoManagement() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    ViewInfo.SubDrawInfo info = new ViewInfo.SubDrawInfo(true, new Rect(), null, null);
    viewInfo.addSubDrawInfo(0, info);
    assertEquals(1, viewInfo.mSubDrawInfoArray.size());

    viewInfo.addSubDrawInfo(-1, info);
    assertEquals(1, viewInfo.mSubDrawInfoArray.size());

    viewInfo.clearSubDrawInfo();
    assertTrue(viewInfo.mSubDrawInfoArray.isEmpty());
  }

  @Test
  public void testClipToRadiusScenarios() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    viewInfo.setClipToRadius(true);

    viewInfo.setClipPathInBeforeDispatchDraw(new Path());
    viewInfo.beforeDispatchDraw(mockCanvas);
    verify(mockCanvas).clipPath(any(Path.class));

    viewInfo.setClipPathInBeforeDispatchDraw(null);
    viewInfo.setClipRectInBeforeDispatchDraw(new Rect(0, 0, 100, 100));
    viewInfo.beforeDispatchDraw(mockCanvas);
    verify(mockCanvas).clipRect(any(Rect.class));
  }

  @Test
  public void testMaskAndBackgroundDrawing() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    MaskDrawable mask = mock(MaskDrawable.class);

    viewInfo.setMaskDrawable(mask);
    viewInfo.afterDraw(mockCanvas);

    verify(mask).draw(any(Canvas.class));
  }

  @Test
  public void testNullHookScenarios() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(null, mockView);
    viewInfo.beforeDraw(mockCanvas);
    verify(mockHook, never()).beforeProcessViewInfo(any());
  }

  @Test
  public void testBackgroundDrawing() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    LynxBackground background = mock(LynxBackground.class);
    BackgroundDrawable drawable = mock(BackgroundDrawable.class);
    when(background.getDrawable()).thenReturn(drawable);

    background.setBackgroundColor(Color.RED);
    ViewInfo.SubDrawInfo info = new ViewInfo.SubDrawInfo(false, new Rect(), null, background);
    viewInfo.addSubDrawInfo(0, info);

    viewInfo.beforeDrawChild(mockCanvas, mockView, 0L);

    verify(background, atLeast(1)).getDrawable();
    verify(drawable).draw(any(Canvas.class));
  }

  @Test
  public void testEdgeCasesInAddSubDrawInfo() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    ViewInfo.SubDrawInfo info = new ViewInfo.SubDrawInfo(true, new Rect(), null, null);

    viewInfo.addSubDrawInfo(3, info);
    assertEquals(4, viewInfo.mSubDrawInfoArray.size()); // [null, null, null, info]

    viewInfo.addSubDrawInfo(1, info);
    assertEquals(5, viewInfo.mSubDrawInfoArray.size());
  }

  @Test
  public void testFullDrawLifecycle() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    viewInfo.setWidth(100);
    viewInfo.setHeight(200);
    viewInfo.setClipPath(new BasicShape(10));

    viewInfo.beforeDraw(mockCanvas);
    viewInfo.beforeDispatchDraw(mockCanvas);
    viewInfo.beforeDrawChild(mockCanvas, mockView, 0L);
    viewInfo.afterDrawChild(mockCanvas, mockView, 0L);
    viewInfo.afterDraw(mockCanvas);

    verify(mockCanvas, times(1)).clipPath(any());
  }

  @Test
  public void testOverflowClipCombination() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    viewInfo.setClipToRadius(true);
    viewInfo.setOverflowClipRect(new Rect(0, 0, 50, 50));
    viewInfo.setClipRectInBeforeDispatchDraw(new Rect(0, 0, 100, 100));

    viewInfo.beforeDispatchDraw(mockCanvas);
    verify(mockCanvas).clipRect(new Rect(0, 0, 100, 100));
    verify(mockCanvas).clipRect(new Rect(0, 0, 50, 50));
  }

  @Test
  public void testDetachLogic() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(mockHook, mockView);

    viewInfo.detachFromUI();
    assertNull(viewInfo.mProcessHook);

    viewInfo.detachWithUI();
  }

  @Test
  public void testNullSafety() {
    IProcessViewInfoHook mockHook = mock(IProcessViewInfoHook.class);
    AndroidView mockView = mock(AndroidView.class);
    Canvas mockCanvas = mock(Canvas.class);

    ViewInfo viewInfo = new ViewInfo(null, null);

    viewInfo.beforeDraw(null);
    viewInfo.beforeDispatchDraw(null);
    viewInfo.beforeDrawChild(null, null, 0L);

    assertTrue(true);
  }
}
