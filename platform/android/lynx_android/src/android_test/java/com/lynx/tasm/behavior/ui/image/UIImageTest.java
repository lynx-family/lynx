// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.graphics.Canvas;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.testing.base.TestingUtils;
import org.junit.Test;

public class UIImageTest {
  @Test
  public void rendererHostDoesNotLoadOrDrawLocalImage() {
    UIImage image = new UIImage(TestingUtils.getLynxContext());
    LynxImageManager manager = mock(LynxImageManager.class);
    image.mLynxImageManager = manager;
    image.getView().setRenderer(mock(Renderer.class));
    Canvas canvas = new Canvas();
    JavaOnlyMap props = new JavaOnlyMap();
    props.putString("src", "https://example.com/image.png");

    image.updatePropertiesInterval(new StylesDiffMap(props));
    image.onNodeReady();
    image.beforeDraw(canvas);

    verify(manager, never()).updatePropertiesInterval(props);
    verify(manager, never()).onNodeReady();
    verify(manager, never()).onDraw(canvas);
  }

  @Test
  public void ordinaryImageStillLoadsAndDraws() {
    UIImage image = new UIImage(TestingUtils.getLynxContext());
    LynxImageManager manager = mock(LynxImageManager.class);
    image.mLynxImageManager = manager;
    Canvas canvas = new Canvas();

    image.onNodeReady();
    image.beforeDraw(canvas);

    verify(manager).onNodeReady();
    verify(manager).onDraw(canvas);
  }

  @Test
  public void pendingRedirectDoesNotLoadAfterRendererAttachment() {
    UIImage image = new UIImage(TestingUtils.getLynxContext());
    LynxImageManager manager = spy(image.mLynxImageManager);
    image.getView().setRenderer(mock(Renderer.class));

    // Redirect completion calls the manager directly, bypassing UIImage.onNodeReady().
    manager.onNodeReady();

    verify(manager, never()).updateNodeProps();
  }
}
