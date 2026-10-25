// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.performance.timing.ITimingCollector;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIBodyViewTest {
  @Test
  @UiThreadTest
  public void clearImageMapDestroysRemainingManagersOnce() {
    UIBody.UIBodyView bodyView = new UIBody.UIBodyView(ApplicationProvider.getApplicationContext());
    LynxImageManager first = mock(LynxImageManager.class);
    LynxImageManager second = mock(LynxImageManager.class);
    bodyView.registerImageAccordingToNodeIndex(1, first);
    bodyView.registerImageAccordingToNodeIndex(2, second);

    bodyView.clearNodeIndexImageMap();
    bodyView.clearNodeIndexImageMap();

    verify(first).destroy();
    verify(second).destroy();
    assertNull(bodyView.peekImageAccordingToNodeIndex(1));
    assertNull(bodyView.peekImageAccordingToNodeIndex(2));
  }

  @Test
  @UiThreadTest
  public void clearImageMapDoesNotDestroyTransferredManager() {
    UIBody.UIBodyView bodyView = new UIBody.UIBodyView(ApplicationProvider.getApplicationContext());
    LynxImageManager transferred = mock(LynxImageManager.class);
    LynxImageManager remaining = mock(LynxImageManager.class);
    bodyView.registerImageAccordingToNodeIndex(1, transferred);
    bodyView.registerImageAccordingToNodeIndex(2, remaining);

    assertSame(transferred, bodyView.obtainImageAccordingToNodeIndex(1));
    assertNull(bodyView.obtainImageAccordingToNodeIndex(1));
    bodyView.clearNodeIndexImageMap();

    verify(transferred, never()).destroy();
    verify(remaining).destroy();
  }

  private static class TestUIBodyView extends UIBody.UIBodyView {
    TestUIBodyView(Context context) {
      super(context);
    }

    void dispatchDrawForTest(Canvas canvas) {
      dispatchDraw(canvas);
    }
  }

  @Test
  public void fragmentLayerDispatchDrawMarksPaintEnd() {
    TestUIBodyView bodyView = new TestUIBodyView(ApplicationProvider.getApplicationContext());
    Renderer renderer = mock(Renderer.class);
    ITimingCollector timingCollector = mock(ITimingCollector.class);
    bodyView.setRenderer(renderer);
    bodyView.setTimingCollector(timingCollector);
    Bitmap bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);

    bodyView.dispatchDrawForTest(canvas);

    verify(renderer).afterDispatchDraw(canvas);
    verify(timingCollector).markPaintEndTimingIfNeeded();
    bitmap.recycle();
  }
}
