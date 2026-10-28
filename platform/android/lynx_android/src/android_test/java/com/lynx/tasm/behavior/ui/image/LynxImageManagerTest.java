// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.*;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;
import androidx.test.annotation.UiThreadTest;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.render.IRendererHost;
import com.lynx.tasm.behavior.render.RoundedRectangle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

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
  public void imageInvalidationUsesRendererHostInsteadOfOrdinaryView() {
    IRendererHost host = mock(IRendererHost.class);
    View view = mock(View.class);
    LynxImageManager manager = new LynxImageManager(mContext);
    manager.setView(view);
    manager.setRendererHost(host);
    manager.invalidate();

    verify(host).invalidateForRenderer();
    verify(view, never()).invalidate();
  }

  @Test
  @UiThreadTest
  public void imageInvalidationStillSupportsOrdinaryViews() {
    View view = mock(View.class);
    LynxImageManager manager = new LynxImageManager(mContext);
    manager.setView(view);
    manager.invalidate();
    verify(view).invalidate();
  }

  @Test
  @UiThreadTest
  public void cuiLoadStartPrecedesLoad() throws Exception {
    assertLoadStartPrecedesLoad(true);
  }

  @Test
  @UiThreadTest
  public void ordinaryLoadStartPrecedesLoad() throws Exception {
    assertLoadStartPrecedesLoad(false);
  }

  private void assertLoadStartPrecedesLoad(boolean cui) throws Exception {
    LynxContext context = spy(mContext);
    EventEmitter emitter = mock(EventEmitter.class);
    when(context.getEventEmitter()).thenReturn(emitter);
    List<LynxCustomEvent> events = new ArrayList<>();
    doAnswer(invocation -> {
      events.add(invocation.getArgument(0));
      return null;
    })
        .when(emitter)
        .sendCustomEvent(any());
    LynxImageManager manager = new LynxImageManager(context);
    if (cui) {
      manager.setFallbackSign(42);
      manager.setEventMask((1 << 5) | LynxImageManager.kFlagImageLoadEvent);
    } else {
      LynxBaseUI ui = mock(LynxBaseUI.class);
      when(ui.getSign()).thenReturn(42);
      manager.setLynxBaseUI(ui);
      Map<String, EventsListener> listeners = new HashMap<>();
      listeners.put("loadstart", mock(EventsListener.class));
      listeners.put("load", mock(EventsListener.class));
      manager.setEvents(listeners);
    }
    manager.setSrc("https://example.com/source.png");
    LynxImageLoader loader = mock(LynxImageLoader.class);
    Field loaderField = LynxImageManager.class.getDeclaredField("mImageLoader");
    loaderField.setAccessible(true);
    loaderField.set(manager, loader);
    doAnswer(invocation -> {
      assertEquals(1, events.size());
      manager.onImageLoadSuccess(100, 80);
      return null;
    })
        .when(loader)
        .fetchImage(any(), any(), any(), any());

    manager.tryFetchImageFromService(100, 80);
    assertEquals(2, events.size());
    assertEquals("loadstart", events.get(0).getName());
    assertEquals(42, events.get(0).getTag());
    assertEquals("https://example.com/source.png", events.get(0).eventParams().get("url"));
    assertEquals("load", events.get(1).getName());
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
  public void fixedModesPreserveExplicitAutoSize() throws Exception {
    LynxImageManager manager = new LynxImageManager(mContext);
    java.lang.reflect.Method autoSize = LynxImageManager.class.getDeclaredMethod("usesAutoSize");
    autoSize.setAccessible(true);
    assertSame(ScalingUtils.ScaleType.FIT_CENTER, manager.getMode("widthFix"));
    assertSame(ScalingUtils.ScaleType.FIT_CENTER, manager.getMode("heightFix"));

    manager.setMode("widthFix");
    manager.setAutoSize(false);
    assertTrue((boolean) autoSize.invoke(manager));
    manager.setMode("heightFix");
    assertTrue((boolean) autoSize.invoke(manager));
    manager.setMode("aspectFit");
    assertFalse((boolean) autoSize.invoke(manager));

    manager.setAutoSize(true);
    manager.setMode("widthFix");
    manager.setMode("aspectFill");
    assertTrue((boolean) autoSize.invoke(manager));
    manager.setAutoSize(false);
    assertFalse((boolean) autoSize.invoke(manager));
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
}
