// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils.ui.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.graphics.PointF;
import android.graphics.RectF;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.utils.ViewHelper;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;

public class ViewHelperTest {
  private LynxContext lynxContext;
  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
  }

  @Test
  public void testViewIsParentOfAnotherView() {
    AndroidView viewA = new AndroidView(lynxContext);
    AndroidView viewB = new AndroidView(lynxContext);

    assertFalse(ViewHelper.viewIsParentOfAnotherView(viewA, viewB));
    assertFalse(ViewHelper.viewIsParentOfAnotherView(viewB, viewA));

    viewA.addView(viewB);
    assertTrue(ViewHelper.viewIsParentOfAnotherView(viewA, viewB));
    assertFalse(ViewHelper.viewIsParentOfAnotherView(viewB, viewA));

    viewA.removeView(viewB);
    viewB.addView(viewA);
    assertTrue(ViewHelper.viewIsParentOfAnotherView(viewB, viewA));
    assertFalse(ViewHelper.viewIsParentOfAnotherView(viewA, viewB));
  }

  @Test
  public void testConvertPoint() {
    AndroidView viewA = new AndroidView(lynxContext);
    viewA.setLeft(0);
    viewA.setRight(1000);
    viewA.setTop(0);
    viewA.setBottom(1000);

    AndroidView viewB = new AndroidView(lynxContext);
    viewB.setLeft(50);
    viewB.setRight(950);
    viewB.setTop(50);
    viewB.setBottom(950);
    viewB.setScaleX(2);
    viewB.setScaleY(2);
    viewB.setRotation(45);
    viewB.setTranslationX(-100);
    viewB.setTranslationY(-100);
    viewA.addView(viewB);

    AndroidView viewC = new AndroidView(lynxContext);
    viewC.setLeft(50);
    viewC.setRight(950);
    viewC.setTop(50);
    viewC.setBottom(950);
    viewB.addView(viewC);

    AndroidView viewD = new AndroidView(lynxContext);
    viewD.setLeft(50);
    viewD.setRight(950);
    viewD.setTop(50);
    viewD.setBottom(950);
    viewC.addView(viewD);

    AndroidView viewE = new AndroidView(lynxContext);
    viewE = spy(viewE);
    when(viewE.getRootView()).thenReturn(viewA);
    viewE.setLeft(50);
    viewE.setRight(950);
    viewE.setTop(50);
    viewE.setBottom(950);
    viewD.addView(viewE);

    PointF point = new PointF(400, 400);
    PointF descendantPoint = ViewHelper.convertPointFromAncestorToDescendant(viewA, viewE, point);
    PointF descendantPoint2 = ViewHelper.convertPointFromViewToAnother(viewA, viewE, point);

    assertEquals(300, descendantPoint.x, 0);
    assertEquals(300.00003, descendantPoint.y, 0.001);
    assertEquals(descendantPoint.x, descendantPoint2.x, 0);
    assertEquals(descendantPoint.y, descendantPoint2.y, 0);

    assertEquals(point.x,
        ViewHelper.convertPointFromDescendantToAncestor(viewE, viewA, descendantPoint).x, 0);
    assertEquals(
        point.x, ViewHelper.convertPointFromViewToAnother(viewE, viewA, descendantPoint).x, 0);
    assertEquals(point.y,
        ViewHelper.convertPointFromDescendantToAncestor(viewE, viewA, descendantPoint).y, 0);
    assertEquals(
        point.y, ViewHelper.convertPointFromViewToAnother(viewE, viewA, descendantPoint).y, 0);

    AndroidView view1 = new AndroidView(lynxContext);
    view1.setLeft(0);
    view1.setRight(1000);
    view1.setTop(0);
    view1.setBottom(1000);

    AndroidView view2 = new AndroidView(lynxContext);
    view2.setLeft(50);
    view2.setRight(950);
    view2.setTop(50);
    view2.setBottom(950);
    view2.setScaleX(2);
    view2.setScaleY(2);
    view2.setRotation(45);
    view2.setTranslationX(-100);
    view2.setTranslationY(-100);
    view1.addView(view2);

    AndroidView view3 = new AndroidView(lynxContext);
    view3.setLeft(50);
    view3.setRight(950);
    view3.setTop(50);
    view3.setBottom(950);
    view2.addView(view3);

    AndroidView view4 = new AndroidView(lynxContext);
    view4.setLeft(50);
    view4.setRight(950);
    view4.setTop(50);
    view4.setBottom(950);
    view3.addView(view4);

    AndroidView view5 = new AndroidView(lynxContext);
    view5 = spy(view5);
    when(view5.getRootView()).thenReturn(view1);
    view5.setLeft(50);
    view5.setRight(950);
    view5.setTop(50);
    view5.setBottom(950);
    view4.addView(view5);

    assertEquals(ViewHelper.convertPointFromViewToAnother(view5, viewE, descendantPoint).x,
        descendantPoint.x, 0);
    assertEquals(ViewHelper.convertPointFromViewToAnother(view5, viewE, descendantPoint).y,
        descendantPoint.y, 0);
    assertEquals(ViewHelper.convertPointFromViewToAnother(viewE, view5, descendantPoint).x,
        descendantPoint.x, 0);
    assertEquals(ViewHelper.convertPointFromViewToAnother(viewE, view5, descendantPoint).y,
        descendantPoint.y, 0);
  }

  @Test
  public void testConvertRect() {
    AndroidView viewA = new AndroidView(lynxContext);
    viewA.setLeft(0);
    viewA.setRight(1000);
    viewA.setTop(0);
    viewA.setBottom(1000);

    AndroidView viewB = new AndroidView(lynxContext);
    viewB.setLeft(50);
    viewB.setRight(950);
    viewB.setTop(50);
    viewB.setBottom(950);
    viewB.setScaleX(2);
    viewB.setScaleY(4);
    viewB.setTranslationX(-100);
    viewB.setTranslationY(-100);
    viewA.addView(viewB);

    AndroidView viewC = new AndroidView(lynxContext);
    viewC.setLeft(50);
    viewC.setRight(950);
    viewC.setTop(50);
    viewC.setBottom(950);
    viewB.addView(viewC);

    AndroidView viewD = new AndroidView(lynxContext);
    viewD.setLeft(50);
    viewD.setRight(950);
    viewD.setTop(50);
    viewD.setBottom(950);
    viewC.addView(viewD);

    AndroidView viewE = new AndroidView(lynxContext);
    viewE = spy(viewE);
    when(viewE.getRootView()).thenReturn(viewA);
    viewE.setLeft(50);
    viewE.setRight(950);
    viewE.setTop(50);
    viewE.setBottom(950);
    viewD.addView(viewE);

    RectF rect = new RectF(0, 100, 400, 200);
    RectF descendantRect = ViewHelper.convertRectFromAncestorToDescendant(viewA, viewE, rect);
    RectF descendantRect2 = ViewHelper.convertRectFromViewToAnother(viewA, viewE, rect);

    assertEquals(100, descendantRect.left, 0.001);
    assertEquals(225, descendantRect.top, 0.001);
    assertEquals(300, descendantRect.right, 0.001);
    assertEquals(250, descendantRect.bottom, 0.001);
    assertEquals(descendantRect.left, descendantRect2.left, 0);
    assertEquals(descendantRect.top, descendantRect2.top, 0);
    assertEquals(descendantRect.right, descendantRect2.right, 0);
    assertEquals(descendantRect.bottom, descendantRect2.bottom, 0);

    assertEquals(rect.left,
        ViewHelper.convertRectFromDescendantToAncestor(viewE, viewA, descendantRect).left, 0);
    assertEquals(rect.top,
        ViewHelper.convertRectFromDescendantToAncestor(viewE, viewA, descendantRect).top, 0);
    assertEquals(rect.right,
        ViewHelper.convertRectFromDescendantToAncestor(viewE, viewA, descendantRect).right, 0);
    assertEquals(rect.bottom,
        ViewHelper.convertRectFromDescendantToAncestor(viewE, viewA, descendantRect).bottom, 0);

    assertEquals(
        rect.left, ViewHelper.convertRectFromViewToAnother(viewE, viewA, descendantRect).left, 0);
    assertEquals(
        rect.top, ViewHelper.convertRectFromViewToAnother(viewE, viewA, descendantRect).top, 0);
    assertEquals(
        rect.right, ViewHelper.convertRectFromViewToAnother(viewE, viewA, descendantRect).right, 0);
    assertEquals(rect.bottom,
        ViewHelper.convertRectFromViewToAnother(viewE, viewA, descendantRect).bottom, 0);

    AndroidView view1 = new AndroidView(lynxContext);
    view1.setLeft(0);
    view1.setRight(1000);
    view1.setTop(0);
    view1.setBottom(1000);

    AndroidView view2 = new AndroidView(lynxContext);
    view2.setLeft(50);
    view2.setRight(950);
    view2.setTop(50);
    view2.setBottom(950);
    view2.setScaleX(2);
    view2.setScaleY(4);
    view2.setTranslationX(-100);
    view2.setTranslationY(-100);
    view1.addView(view2);

    AndroidView view3 = new AndroidView(lynxContext);
    view3.setLeft(50);
    view3.setRight(950);
    view3.setTop(50);
    view3.setBottom(950);
    view2.addView(view3);

    AndroidView view4 = new AndroidView(lynxContext);
    view4.setLeft(50);
    view4.setRight(950);
    view4.setTop(50);
    view4.setBottom(950);
    view3.addView(view4);

    AndroidView view5 = new AndroidView(lynxContext);
    view5 = spy(view5);
    when(view5.getRootView()).thenReturn(view1);
    view5.setLeft(50);
    view5.setRight(950);
    view5.setTop(50);
    view5.setBottom(950);
    view4.addView(view5);

    assertEquals(ViewHelper.convertRectFromViewToAnother(view5, viewE, descendantRect).left,
        descendantRect.left, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(view5, viewE, descendantRect).top,
        descendantRect.top, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(view5, viewE, descendantRect).right,
        descendantRect.right, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(view5, viewE, descendantRect).bottom,
        descendantRect.bottom, 0);

    assertEquals(ViewHelper.convertRectFromViewToAnother(viewE, view5, descendantRect).left,
        descendantRect.left, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(viewE, view5, descendantRect).top,
        descendantRect.top, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(viewE, view5, descendantRect).right,
        descendantRect.right, 0);
    assertEquals(ViewHelper.convertRectFromViewToAnother(viewE, view5, descendantRect).bottom,
        descendantRect.bottom, 0);
  }
}
