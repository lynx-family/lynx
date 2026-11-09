// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.*;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;
import androidx.test.annotation.UiThreadTest;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.render.RoundedRectangle;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

public class LynxImageManagerTest {
  private LynxContext mContext;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
  }

  @After
  public void tearDown() {}

  @Test
  @UiThreadTest
  public void imageInvalidationStillSupportsOrdinaryViews() {
    View view = mock(View.class);
    LynxImageManager manager = new LynxImageManager(mContext);
    manager.setView(view);
    manager.invalidate();
    verify(view).invalidate();
  }

  private BackgroundDrawable.RoundRectPath getRoundRectPath(LynxImageManager manager) {
    BackgroundDrawable.RoundRectPath path = null;
    try {
      Field field = LynxImageManager.class.getDeclaredField("mInnerClipPathForBorderRadius");
      field.setAccessible(true);
      path = (BackgroundDrawable.RoundRectPath) field.get(manager);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail(e.toString());
    }
    return path;
  }

  private Path getPath(BackgroundDrawable.RoundRectPath roundRectPath) {
    Path path = null;
    try {
      Field field = BackgroundDrawable.RoundRectPath.class.getDeclaredField("path");
      field.setAccessible(true);
      path = (Path) field.get(roundRectPath);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail(e.toString());
    }
    return path;
  }

  private RectF convertToRectF(Rect rect) {
    return new RectF(rect.left, rect.top, rect.right, rect.bottom);
  }

  private boolean isPathEqual(Path p1, Path p2) {
    if (p1 == p2)
      return true;

    Path result = new Path();
    // Perform XOR operation between p1 and p2, store the result in 'result'
    // If the two shapes are identical, the XOR result should be empty
    boolean success = result.op(p1, p2, Path.Op.XOR);

    if (success) {
      return result.isEmpty();
    }
    return false;
  }

  @Test
  public void getModeFromInt() {
    LynxImageManager manager = new LynxImageManager(mContext);

    assertSame(ScalingUtils.ScaleType.FIT_XY, manager.getMode(0));
    assertSame(ScalingUtils.ScaleType.FIT_CENTER, manager.getMode(1));
    assertSame(ScalingUtils.ScaleType.CENTER_CROP, manager.getMode(2));
    assertSame(ScalingUtils.ScaleType.CENTER, manager.getMode(3));
    assertSame(ScalingUtils.ScaleType.FIT_XY, manager.getMode(-1));
  }

  @Test
  public void updateInnerClipPathForBorderRadius() {
    RectF rectF = new RectF(1.1f, 1.1f, 10.2f, 10.2f);
    float[] borders = new float[8];
    borders[0] = 1.0f;
    borders[1] = 1.0f;
    borders[2] = 1.0f;
    borders[3] = 1.0f;
    borders[4] = 1.0f;
    borders[5] = 1.0f;
    borders[6] = 1.0f;
    borders[7] = 1.0f;
    RoundedRectangle roundedRect = new RoundedRectangle(rectF, borders);

    LynxImageManager manager = new LynxImageManager(mContext);
    manager.updateInnerClipPathForBorderRadius(roundedRect);
    BackgroundDrawable.RoundRectPath roundRectPath = getRoundRectPath(manager);
    assertNotNull(roundRectPath);

    Path path = getPath(roundRectPath);
    Path expectPath = new Path();
    expectPath.addRoundRect(
        convertToRectF(roundedRect.getRect()), roundedRect.getBorderRadii(), Path.Direction.CW);
    assertTrue(isPathEqual(path, expectPath));

    manager.updateInnerClipPathForBorderRadius(null);
    roundRectPath = getRoundRectPath(manager);
    assertNull(roundRectPath);
  }

  private LynxImageManager reloadManager(EventEmitter emitter) {
    mContext.setEventEmitter(emitter);
    LynxImageManager manager = new LynxImageManager(mContext);
    manager.setFallbackSign(42);
    manager.setEventMask(
        LynxImageManager.kFlagImageLoadEvent | LynxImageManager.kFlagImageErrorEvent);
    manager.setSrc("https://example.com/image.png");
    // Zero size defers the request, but consumes the initial property changes.
    manager.onNodeReady();
    return manager;
  }

  @Test
  @UiThreadTest
  public void reloadReplaysLoadOnlyAfterReloadLifecycle() {
    EventEmitter emitter = mock(EventEmitter.class);
    LynxImageManager manager = reloadManager(emitter);
    manager.onImageLoadSuccess(80, 40);
    manager.onPatchFinish();
    verify(emitter).sendCustomEvent(any(LynxCustomEvent.class));

    manager.onNodeReload();
    manager.setSrc("https://example.com/image.png");
    manager.onPatchFinish();
    manager.onPatchFinish();
    ArgumentCaptor<LynxCustomEvent> events = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(emitter, times(2)).sendCustomEvent(events.capture());
    assertEquals("load", events.getValue().getName());
    assertEquals(80, events.getValue().eventParams().get("width"));
    assertEquals(40, events.getValue().eventParams().get("height"));

    // A later size-dependent request retains its normal callback behavior.
    manager.onImageLoadSuccess(160, 80);
    verify(emitter, times(3)).sendCustomEvent(any(LynxCustomEvent.class));
  }

  @Test
  @UiThreadTest
  public void reloadReplaysErrorDetails() {
    EventEmitter emitter = mock(EventEmitter.class);
    LynxImageManager manager = reloadManager(emitter);
    LynxError error = new LynxError(1, "Image failed", "", LynxError.LEVEL_ERROR);
    error.setRootCause("missing image");
    manager.onImageLoadError(error, 2, 3);
    manager.onNodeReload();
    manager.onPatchFinish();
    ArgumentCaptor<LynxCustomEvent> events = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(emitter, times(2)).sendCustomEvent(events.capture());
    assertEquals("error", events.getValue().getName());
    assertEquals(events.getAllValues().get(0).eventParams(), events.getValue().eventParams());
  }

  @Test
  @UiThreadTest
  public void reloadWithNewSourceWaitsForResult() {
    EventEmitter emitter = mock(EventEmitter.class);
    LynxImageManager manager = reloadManager(emitter);
    manager.onImageLoadSuccess(80, 40);
    manager.onNodeReload();
    manager.setSrc("https://example.com/new-image.png");
    manager.onPatchFinish();
    verify(emitter).sendCustomEvent(any(LynxCustomEvent.class));
    manager.onNodeReady();
    // A real result arriving during reload must not be replayed again.
    manager.onNodeReload();
    manager.onImageLoadSuccess(160, 80);
    manager.onPatchFinish();
    verify(emitter, times(2)).sendCustomEvent(any(LynxCustomEvent.class));
  }
}
