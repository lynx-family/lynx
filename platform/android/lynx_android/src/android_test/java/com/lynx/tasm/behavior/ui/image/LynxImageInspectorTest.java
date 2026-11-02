// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.LynxScaleTypeDrawable;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageAnimationInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;

public class LynxImageInspectorTest {
  private LynxImageManager mManager;

  @Before
  public void setUp() {
    mManager = new LynxImageManager(TestingUtils.getLynxContext());
    LynxBaseUI ui = mock(LynxBaseUI.class);
    when(ui.getLynxContext()).thenReturn(TestingUtils.getLynxContext());
    mManager.setLynxBaseUI(ui);
  }

  @Test
  public void loadingAndSourceChangesLeaveUnknownStateNullable() {
    JavaOnlyMap snapshot = mManager.getInspectorSnapshot();
    assertEquals("loading", snapshot.getString("state"));
    assertTrue(snapshot.isNull("intrinsicWidth"));
    assertTrue(snapshot.isNull("frameIndex"));
    assertTrue(snapshot.isNull("frameCount"));
    mManager.setSrc("first.png");
    mManager.setSrc("first.png");
    assertEquals(1, mManager.getInspectorSnapshot().getLong("resourceGeneration"));
    mManager.setSrc("second.png");
    assertEquals(2, mManager.getInspectorSnapshot().getLong("resourceGeneration"));
  }

  @Test
  public void playbackAndMetadataAreReadFromCurrentDrawable() throws Exception {
    Drawable drawable =
        mock(Drawable.class, org.mockito.Mockito.withSettings().extraInterfaces(Animatable.class));
    LynxImageLoader loader = mock(LynxImageLoader.class);
    when(loader.getAnimationInfo(drawable)).thenReturn(new ImageAnimationInfo(24, 1000));
    setField("mImageLoader", loader);
    setField("mImageDrawable",
        new LynxScaleTypeDrawable(new ImageContent(drawable), ScalingUtils.ScaleType.FIT_XY));
    setField("mInspectorResourceState", "ready");
    setField("mImageWidth", 40);
    AnimationListener listener = (AnimationListener) getField("mAnimationListener");
    listener.onAnimationFrame(drawable, 12);
    JavaOnlyMap snapshot = mManager.getInspectorSnapshot();
    assertEquals(12, snapshot.getInt("frameIndex"));
    assertEquals(24, snapshot.getInt("frameCount"));
    assertEquals(40, snapshot.getInt("intrinsicWidth"));
    assertFalse(snapshot.getBoolean("isAnimating"));
    when(((Animatable) drawable).isRunning()).thenReturn(true);
    assertTrue(mManager.getInspectorSnapshot().getBoolean("isAnimating"));
    listener.onAnimationFrame(mock(Drawable.class), 3);
    assertEquals(12, mManager.getInspectorSnapshot().getInt("frameIndex"));
    mManager.setSrc("next.png");
    listener.onAnimationFrame(drawable, 13);
    assertTrue(mManager.getInspectorSnapshot().isNull("frameIndex"));
  }

  @Test
  public void resourceFailureReportsCodeWithoutProviderText() throws Exception {
    ImageLoadListener listener = (ImageLoadListener) getField("mSrcLoadListenerInner");
    listener.onFailure(17, new RuntimeException("private resource details"));
    JavaOnlyMap snapshot = mManager.getInspectorSnapshot();
    assertEquals("error", snapshot.getString("state"));
    assertEquals(17, snapshot.getInt("errorCode"));
    assertFalse(snapshot.toString().contains("private resource details"));
    mManager.setSrc("next.png");
    assertTrue(mManager.getInspectorSnapshot().isNull("errorCode"));
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
