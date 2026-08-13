// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.KeyEvent;
import android.view.View;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import org.junit.Test;

public class LynxViewInputEventTest {
  @Test
  public void testConsumedRendererPreservesDispatchResult() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    ILynxUIRenderer renderer = mock(ILynxUIRenderer.class);
    when(renderer.needHandleDispatchKeyEvent()).thenReturn(true);
    KeyEvent event = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER);
    when(renderer.dispatchKeyEvent(event)).thenReturn(true);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      TestLynxView view = new TestLynxView(context, renderer);
      assertTrue(view.dispatchKeyEvent(event));
      verify(renderer).dispatchKeyEvent(event);
    });
  }

  @Test
  public void testNestedLynxViewObservesKeyOnlyOnceAndContinuesDispatch() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    ILynxUIRenderer parentRenderer = mock(ILynxUIRenderer.class);
    ILynxUIRenderer childRenderer = mock(ILynxUIRenderer.class);
    when(parentRenderer.needHandleDispatchKeyEvent()).thenReturn(true);
    when(childRenderer.needHandleDispatchKeyEvent()).thenReturn(true);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      TestLynxView parent = new TestLynxView(context, parentRenderer);
      TestLynxView child = new TestLynxView(context, childRenderer);
      KeyHandlingView focusedView = new KeyHandlingView(context);
      focusedView.setFocusableInTouchMode(true);
      child.addView(focusedView);
      parent.addView(child);
      int measureSpec = View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY);
      parent.measure(measureSpec, measureSpec);
      parent.layout(0, 0, 100, 100);
      child.layout(0, 0, 100, 100);
      focusedView.layout(0, 0, 100, 100);
      assertTrue(focusedView.requestFocus());

      KeyEvent event = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER);
      parent.dispatchKeyEvent(event);

      verify(parentRenderer, never()).dispatchKeyEvent(event);
      verify(childRenderer).dispatchKeyEvent(event);
      assertEquals(1, focusedView.mKeyEventCount);
    });
  }

  private static class TestLynxView extends LynxView {
    private final ILynxUIRenderer mRenderer;

    TestLynxView(Context context, ILynxUIRenderer renderer) {
      super(context);
      mRenderer = renderer;
    }

    @Override
    public ILynxUIRenderer lynxUIRenderer() {
      return mRenderer;
    }
  }

  private static class KeyHandlingView extends View {
    private int mKeyEventCount;

    KeyHandlingView(Context context) {
      super(context);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
      mKeyEventCount++;
      return true;
    }
  }
}
