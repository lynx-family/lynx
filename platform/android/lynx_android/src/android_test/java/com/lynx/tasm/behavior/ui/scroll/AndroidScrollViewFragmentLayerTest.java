// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.view.View;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;
import com.lynx.tasm.behavior.ui.utils.PlatformLength;
import com.lynx.testing.base.TestingUtils;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class AndroidScrollViewFragmentLayerTest {
  private static final int VIEWPORT_SIZE = 100;
  private static final int BITMAP_WIDTH = 140;
  private static final int BORDER_RADIUS = 40;

  private static class TestAndroidScrollView extends AndroidScrollView {
    TestAndroidScrollView(Context context) {
      super(context, mock(UIScrollView.class));
    }

    void dispatchDrawForTest(Canvas canvas) {
      dispatchDraw(canvas);
    }
  }

  @Test
  public void fragmentLayerContentIsClippedToHorizontalScrollViewportAndBorderRadius() {
    Context context = ApplicationProvider.getApplicationContext();
    TestAndroidScrollView scrollView = new TestAndroidScrollView(context);
    scrollView.setOrientation(AndroidScrollView.HORIZONTAL);
    scrollView.measure(View.MeasureSpec.makeMeasureSpec(VIEWPORT_SIZE, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(VIEWPORT_SIZE, View.MeasureSpec.EXACTLY));
    scrollView.layout(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    scrollView.getHScrollView().scrollTo(30, 0);

    BackgroundDrawable background = new BackgroundDrawable(TestingUtils.getLynxContext(), 0);
    background.setBounds(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    for (int corner = 1; corner <= 4; corner++) {
      background.setBorderRadiusCorner(corner, createBorderRadiusCorner(BORDER_RADIUS));
    }
    scrollView.setBackground(background);

    Renderer renderer = mock(Renderer.class);
    doAnswer(invocation -> {
      Canvas canvas = invocation.getArgument(0);
      canvas.drawColor(Color.RED);
      return null;
    })
        .when(renderer)
        .afterDispatchDraw(any(Canvas.class));
    scrollView.setRenderer(renderer);

    Bitmap bitmap = Bitmap.createBitmap(BITMAP_WIDTH, VIEWPORT_SIZE, Bitmap.Config.ARGB_8888);
    scrollView.dispatchDrawForTest(new Canvas(bitmap));

    assertEquals("FLR content inside the rounded viewport should be visible", Color.RED,
        bitmap.getPixel(VIEWPORT_SIZE / 2, VIEWPORT_SIZE / 2));
    assertEquals("FLR content should be clipped at rounded corners", Color.TRANSPARENT,
        bitmap.getPixel(0, 0));
    assertEquals("FLR content should not overflow the horizontal scroll viewport",
        Color.TRANSPARENT, bitmap.getPixel(VIEWPORT_SIZE + 10, VIEWPORT_SIZE / 2));
    bitmap.recycle();
  }

  private static BorderRadius.Corner createBorderRadiusCorner(float radius) {
    BorderRadius.Corner corner = new BorderRadius.Corner();
    corner.x = new PlatformLength(radius, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    corner.y = new PlatformLength(radius, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    return corner;
  }
}
