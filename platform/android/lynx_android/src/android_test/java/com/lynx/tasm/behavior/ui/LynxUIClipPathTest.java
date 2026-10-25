// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxUIClipPathTest {
  private static final int SIZE = 100;

  @Test
  @UiThreadTest
  public void clipsChildrenWithoutBackground() {
    for (boolean useViewInfo : new boolean[] {false, true}) {
      Fixture fixture = new Fixture(useViewInfo);
      fixture.assertPixels(Color.RED, Color.RED);
      fixture.ui.setClipPath(rightHalfClip());
      fixture.assertPixels(Color.TRANSPARENT, Color.RED);
    }
  }

  @Test
  @UiThreadTest
  public void clipsTransformedChildrenWithoutBackground() {
    for (boolean useViewInfo : new boolean[] {false, true}) {
      Fixture fixture = new Fixture(useViewInfo);
      fixture.child.setRotation(45);
      fixture.ui.setClipPath(rightHalfClip());
      fixture.assertPixels(Color.TRANSPARENT, Color.RED);
    }
  }

  @Test
  @UiThreadTest
  public void clipsBackgroundAndChildren() {
    for (boolean useViewInfo : new boolean[] {false, true}) {
      Fixture fixture = new Fixture(useViewInfo);
      fixture.ui.getView().setBackgroundColor(Color.GREEN);
      fixture.child.layout(0, 0, SIZE, SIZE / 2);
      fixture.ui.setClipPath(rightHalfClip());
      fixture.assertPixels(Color.TRANSPARENT, Color.GREEN);
    }
  }

  @Test
  @UiThreadTest
  public void stillClipsAfterBackgroundIsRemoved() {
    for (boolean useViewInfo : new boolean[] {false, true}) {
      Fixture fixture = new Fixture(useViewInfo);
      fixture.ui.getView().setBackgroundColor(Color.GREEN);
      fixture.ui.setClipPath(rightHalfClip());
      fixture.assertPixels(Color.TRANSPARENT, Color.RED);
      fixture.ui.getView().setBackground(null);
      fixture.assertPixels(Color.TRANSPARENT, Color.RED);
    }
  }

  @Test
  @UiThreadTest
  public void removingClipRestoresChildren() {
    for (boolean useViewInfo : new boolean[] {false, true}) {
      for (boolean resetWithNull : new boolean[] {false, true}) {
        Fixture fixture = new Fixture(useViewInfo);
        fixture.ui.setClipPath(rightHalfClip());
        fixture.assertPixels(Color.TRANSPARENT, Color.RED);
        fixture.ui.setClipPath(resetWithNull ? null : new JavaOnlyArray());
        fixture.assertPixels(Color.RED, Color.RED);
      }
    }
  }

  @Test
  @UiThreadTest
  public void resettingClipPreservesOtherDrawingRequirements() {
    Fixture fixture = new Fixture(false);
    fixture.ui.getView().setWillNotDraw(false);
    fixture.ui.setClipPath(rightHalfClip());
    fixture.ui.setClipPath(null);
    assertFalse(fixture.ui.getView().willNotDraw());
  }

  @Test
  @UiThreadTest
  public void restoresClipDrawingRequirementWhenViewIsRecreated() {
    Fixture fixture = new Fixture(false);
    AndroidView view = fixture.ui.getView();
    fixture.ui.mView = null;
    fixture.ui.setClipPath(rightHalfClip());
    fixture.ui.mView = view;
    fixture.ui.didEnsureCreateView();
    assertFalse(view.willNotDraw());
    fixture.assertPixels(Color.TRANSPARENT, Color.RED);
  }

  private static JavaOnlyArray rightHalfClip() {
    JavaOnlyArray shape = new JavaOnlyArray();
    shape.pushInt(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    for (int edge = 0; edge < 4; edge++) {
      shape.pushDouble(edge == 3 ? 0.5 : 0);
      shape.pushInt(StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
    }
    return shape;
  }

  private static class Fixture {
    final UIView ui;
    final View child;
    final FrameLayout root;

    Fixture(boolean useViewInfo) {
      LynxContext context = TestingUtils.getLynxContext();
      ui = new UIView(context) {
        @Override
        public int getWidth() {
          return SIZE;
        }

        @Override
        public int getHeight() {
          return SIZE;
        }
      };
      AndroidView container = ui.getView();
      if (useViewInfo) {
        container.bindDrawChildHook(new ViewInfo(ui, container));
      }
      child = new View(context);
      child.setBackgroundColor(Color.RED);
      container.addView(child);
      root = new FrameLayout(context);
      root.addView(container, new FrameLayout.LayoutParams(SIZE, SIZE));
      int spec = View.MeasureSpec.makeMeasureSpec(SIZE, View.MeasureSpec.EXACTLY);
      root.measure(spec, spec);
      root.layout(0, 0, SIZE, SIZE);
      child.layout(0, 0, SIZE, SIZE);
    }

    void assertPixels(int leftColor, int rightColor) {
      Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
      try {
        // Draw through an Android parent: directly calling container.draw() bypasses skip-draw.
        root.draw(new Canvas(bitmap));
        assertEquals("Pixel outside clip", leftColor, bitmap.getPixel(SIZE / 4, SIZE / 2));
        assertEquals("Pixel inside clip", rightColor, bitmap.getPixel(SIZE * 3 / 4, SIZE / 2));
      } finally {
        bitmap.recycle();
      }
    }
  }
}
