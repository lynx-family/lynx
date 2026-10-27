// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.view.TextureView;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.animax.ability.BaseAbility;
import com.lynx.animax.ui.AnimaXContainerView;
import com.lynx.animax.ui.AnimaXContext;
import com.lynx.animax.ui.AnimaXImageView;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.render.PlatformRendererContext;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.UIBody;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIAnimaXFragmentLayerTest {
  private Context mContext;

  @Before
  public void setUp() {
    mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxEnv.inst().init((Application) mContext.getApplicationContext(), null, null, null, null);
  }

  @Test
  public void fallbackKeepsAnimaXUIAndContainer() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext context = new LynxContext(mContext, mContext.getResources().getDisplayMetrics()) {
        @Override
        public void handleException(Exception exception) {
          throw new AssertionError(exception);
        }
      };
      UIAnimaX ui = new UIAnimaX(context);
      assertTrue(ui.getView() instanceof LynxAnimaXContainerView);
      LynxAnimaXContainerView container = (LynxAnimaXContainerView) ui.getView();
      LynxUIOwner owner = mock(LynxUIOwner.class);
      context.setLynxUIOwner(owner);
      when(owner.getNode(1)).thenReturn(ui);
      BehaviorRegistry registry = new BehaviorRegistry();
      registry.addBehavior(new Behavior("animax"));
      PlatformRendererContext rendererContext =
          new PlatformRendererContext(mock(UIBody.UIBodyView.class), context, registry);
      try {
        rendererContext.createPlatformExtendedRenderer(1, "animax", null);

        verify(owner).createView(1, "animax", null, null, null, false, 1, null);
        verify(owner, never()).cleanupCreatedView(1, "animax", null);
        Renderer renderer = container.getRenderer();
        assertNotNull(renderer);
        assertSame(ui, renderer.getUIHost());
        assertSame(container, renderer.getRendererHost());
        assertSame(container, ui.getView());

        rendererContext.updatePlatformRendererFrame(
            1, true, 10, 20, 100, 80, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        assertEquals(new Rect(13, 24, 113, 104), renderer.getLynxFrame());
        container.applyRendererOpacity(0.5f);
        assertEquals(0.5f, container.getAlpha(), 0.001f);
      } finally {
        rendererContext.destroy();
        ui.destroy();
      }
    });
  }

  @Test
  public void nativeChildrenFollowContainerSizeAndPadding() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxAnimaXContainerView container = new LynxAnimaXContainerView(mContext);
      TextureView texture = new TextureView(mContext);
      View image = new View(mContext);
      container.addView(texture,
          new FrameLayout.LayoutParams(
              FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
      container.addView(image,
          new FrameLayout.LayoutParams(
              FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
      container.setPadding(7, 9, 11, 13);

      measureAndLayout(container, 100, 80);
      assertEquals(new Rect(7, 9, 89, 67), bounds(texture));
      assertEquals(bounds(texture), bounds(image));

      container.setRenderer(new Renderer(mock(PlatformRendererContext.class), 1));
      measureAndLayout(container, 160, 120);
      assertEquals(new Rect(7, 9, 149, 107), bounds(texture));
      assertEquals(bounds(texture), bounds(image));
      assertEquals(142, texture.getMeasuredWidth());
      assertEquals(98, image.getMeasuredHeight());
    });
  }

  @Test
  public void rendererPreservesNativeScreenshotAndClipping() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxAnimaXContainerView container = new LynxAnimaXContainerView(mContext);
      container.setRenderer(new Renderer(mock(PlatformRendererContext.class), 1));
      container.setWillNotDraw(false);
      Bitmap screenshot = Bitmap.createBitmap(80, 60, Bitmap.Config.ARGB_8888);
      screenshot.eraseColor(Color.BLUE);
      Bitmap output = Bitmap.createBitmap(80, 60, Bitmap.Config.ARGB_8888);
      output.eraseColor(Color.WHITE);
      AnimaXPlayer player = mock(AnimaXPlayer.class);
      when(player.getAnimaXContext())
          .thenReturn(new AnimaXContext.Builder(mock(BaseAbility.class), mContext).build());
      AnimaXImageView image = new AnimaXImageView(player) {
        @Override
        protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {}

        @Override
        protected void onDraw(Canvas canvas) {
          canvas.drawColor(Color.GREEN);
        }

        @Override
        public Bitmap getCurrentBitmap() {
          return screenshot;
        }
      };
      container.addChildAnimaXView(image, new AnimaXContainerView.IDispatchDrawHook() {
        @Override
        public void beforeDispatchDraw(Canvas canvas) {
          canvas.clipRect(0, 0, 20, 40);
        }

        @Override
        public boolean useBitmapOnDraw() {
          return true;
        }
      });
      try {
        measureAndLayout(container, 80, 60);
        Canvas canvas = new Canvas(output);
        container.draw(canvas);
        assertEquals(Color.BLUE, output.getPixel(10, 10));
        assertEquals(Color.WHITE, output.getPixel(40, 50));
      } finally {
        image.release();
        screenshot.recycle();
        output.recycle();
      }
    });
  }

  private static void measureAndLayout(View view, int width, int height) {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
  }

  private static Rect bounds(View view) {
    return new Rect(view.getLeft(), view.getTop(), view.getRight(), view.getBottom());
  }
}
