// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.LynxScaleTypeDrawable;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageAnimationInfo;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;

public class LynxImageAnimationQueryTest {
  private LynxImageManager mManager;
  private LynxImageLoader mLoader;

  @Before
  public void setUp() throws Exception {
    mManager = new LynxImageManager(TestingUtils.getLynxContext());
    LynxBaseUI ui = mock(LynxBaseUI.class);
    when(ui.getLynxContext()).thenReturn(TestingUtils.getLynxContext());
    mManager.setLynxBaseUI(ui);
    mLoader = mock(LynxImageLoader.class);
    setField("mImageLoader", mLoader);
  }

  @Test
  public void unknownMetadataDoesNotBecomeAnInventedFrame() {
    assertNull(mManager.getCurrentFrame());
    assertNull(mManager.getFrameCount());
    assertNull(mManager.getDuration());
    assertFalse(mManager.isAnimating());
  }

  @Test
  public void nativePlaybackChangesAreVisibleWithoutControlStateTracking() throws Exception {
    Drawable drawable = mock(Drawable.class, withSettings().extraInterfaces(Animatable.class));
    when(mLoader.getAnimationInfo(drawable)).thenReturn(new ImageAnimationInfo(24, 1000));
    setDrawable(drawable);
    assertEquals(Integer.valueOf(24), mManager.getFrameCount());
    assertEquals(Long.valueOf(1000), mManager.getDuration());
    assertFalse(mManager.isAnimating());
    when(((Animatable) drawable).isRunning()).thenReturn(true);
    assertTrue(mManager.isAnimating());
    when(((Animatable) drawable).isRunning()).thenReturn(false);
    assertFalse(mManager.isAnimating());
    verify((Animatable) drawable, never()).start();
    verify((Animatable) drawable, never()).stop();
  }

  @Test
  public void replacementDoesNotReuseFramesFromPreviousDrawable() throws Exception {
    Drawable previous = mock(Drawable.class);
    Drawable current = mock(Drawable.class);
    setDrawable(previous);
    AnimationListener listener = (AnimationListener) getField("mAnimationListener");
    listener.onAnimationFrame(previous, 12);
    assertEquals(Integer.valueOf(12), mManager.getCurrentFrame());
    setDrawable(current);
    assertNull(mManager.getCurrentFrame());
    listener.onAnimationFrame(previous, 13);
    assertNull(mManager.getCurrentFrame());
    listener.onAnimationFrame(current, 0);
    assertEquals(Integer.valueOf(0), mManager.getCurrentFrame());
  }

  @Test
  public void componentDispatchReturnsNativeValuesAndRequiresDevTool() throws Exception {
    com.lynx.devtoolwrapper.DevToolSettings settings = com.lynx.devtoolwrapper.DevToolSettings.inst();
    settings.init(TestingUtils.getLynxContext());
    com.lynx.devtoolwrapper.DevToolLifecycle lifecycle = com.lynx.devtoolwrapper.DevToolLifecycle.getInstance();
    Field state = lifecycle.getClass().getDeclaredField("mState");
    state.setAccessible(true);
    Object previous = state.get(lifecycle);
    boolean enabled = settings.isDevToolEnabled();
    try {
      state.set(lifecycle, com.lynx.devtoolwrapper.DevToolState.ENABLED);
      settings.setDevToolEnabled(true);
      UIImage ui = mock(UIImage.class, CALLS_REAL_METHODS);
      FlattenUIImage flat = mock(FlattenUIImage.class, CALLS_REAL_METHODS);
      LynxImageManager manager = mock(LynxImageManager.class);
      ui.mLynxImageManager = manager;
      flat.mLynxImageManager = manager;
      when(manager.getCurrentFrame()).thenReturn(7);
      when(manager.getFrameCount()).thenReturn(24);
      when(manager.getDuration()).thenReturn(1000L);
      when(manager.isAnimating()).thenReturn(true);
      String[] methods = {"getCurrentFrame", "getFrameCount", "getDuration", "isAnimating"};
      Object[] values = {7, 24, 1000L, true};
      for (int index = 0; index < methods.length; index++) {
        com.lynx.react.bridge.Callback callback = mock(com.lynx.react.bridge.Callback.class);
        new UIImage$$MethodInvoker().invoke(ui, methods[index], null, callback);
        verify(callback).invoke(0, values[index]);
        callback = mock(com.lynx.react.bridge.Callback.class);
        new FlattenUIImage$$MethodInvoker().invoke(flat, methods[index], null, callback);
        verify(callback).invoke(0, values[index]);
      }
      settings.setDevToolEnabled(false);
      com.lynx.react.bridge.Callback callback = mock(com.lynx.react.bridge.Callback.class);
      new UIImage$$MethodInvoker().invoke(ui, "getCurrentFrame", null, callback);
      verify(callback).invoke(com.lynx.tasm.behavior.LynxUIMethodConstants.METHOD_NOT_FOUND);
    } finally {
      settings.setDevToolEnabled(enabled);
      state.set(lifecycle, previous);
    }
  }

  private void setDrawable(Drawable drawable) throws Exception {
    setField("mImageDrawable", new LynxScaleTypeDrawable(new ImageContent(drawable), ScalingUtils.ScaleType.FIT_XY));
  }

  private Object getField(String name) throws Exception {
    Field field = LynxImageManager.class.getDeclaredField(name);
    field.setAccessible(true);
    return field.get(mManager);
  }

  private void setField(String name, Object value) throws Exception {
    Field field = LynxImageManager.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(mManager, value);
  }
}
