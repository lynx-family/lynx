// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.View.MeasureSpec;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests the engine-side Host Script View hook bridge without requiring the optional library.
 */
@RunWith(AndroidJUnit4.class)
public class LynxHostScriptViewHooksBridgeTest {
  private LynxHostScriptViewHooks.Delegate mPreviousDelegate;

  @Before
  public void setUp() {
    mPreviousDelegate = LynxHostScriptViewHooks.getDelegate();
    LynxHostScriptViewHooks.setDelegate(null);
  }

  @After
  public void tearDown() {
    LynxHostScriptViewHooks.setDelegate(mPreviousDelegate);
  }

  @Test
  public void missingDelegateLeavesAllRequestsUntouched() {
    LynxLoadMeta.Builder loadBuilder = new LynxLoadMeta.Builder();
    loadBuilder.setUrl("assets://original.js");
    LynxLoadMeta load = loadBuilder.build();
    TemplateData props = TemplateData.fromString("{\"theme\":\"light\"}");
    LynxUpdateMeta update = new LynxUpdateMeta.Builder().setUpdatedGlobalProps(props).build();
    LynxViewBuilder builder = new LynxViewBuilder();
    int width = MeasureSpec.makeMeasureSpec(720, MeasureSpec.EXACTLY);
    int height = MeasureSpec.makeMeasureSpec(1280, MeasureSpec.AT_MOST);

    assertFalse(LynxHostScriptViewHooks.isEnabled());
    assertSame(load, LynxHostScriptViewHooks.beforeLoadTemplate(null, load));
    assertSame(update, LynxHostScriptViewHooks.beforeUpdateMetaData(null, update));
    assertSame(props, LynxHostScriptViewHooks.beforeUpdateGlobalProps(null, props));
    assertSame(builder, LynxHostScriptViewHooks.beforeCreate(null, builder));
    long packed = LynxHostScriptViewHooks.beforeUpdateViewport(null, width, height);
    assertEquals(width, (int) (packed >>> 32));
    assertEquals(height, (int) packed);
    LynxHostScriptViewHooks.register(null, null);
    LynxHostScriptViewHooks.unregister(null);
    LynxHostScriptViewHooks.clearViewUrl(1L);
  }

  @Test
  public void registeredDelegateReceivesViewIdAndOverridesResults() {
    LynxHostScriptViewHooks.Delegate delegate = mock(LynxHostScriptViewHooks.Delegate.class);
    LynxHostScriptViewHooks.setDelegate(delegate);

    LynxView view = mock(LynxView.class);
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    when(render.getLogContextSnapshot()).thenReturn(new LynxLogContext(42L, 1L, 2L));

    LynxLoadMeta originalLoad = new LynxLoadMeta.Builder().build();
    LynxLoadMeta patchedLoad = new LynxLoadMeta.Builder().build();
    LynxUpdateMeta originalUpdate = new LynxUpdateMeta.Builder().build();
    LynxUpdateMeta patchedUpdate = new LynxUpdateMeta.Builder().build();
    TemplateData originalProps = TemplateData.fromString("{\"a\":1}");
    TemplateData patchedProps = TemplateData.fromString("{\"a\":2}");
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxViewBuilder originalBuilder = new LynxViewBuilder();
    LynxViewBuilder patchedBuilder = new LynxViewBuilder();
    long patchedSpecs = (100L << 32) | 200L;

    when(delegate.isEnabled()).thenReturn(true);
    when(delegate.beforeCreate(context, originalBuilder)).thenReturn(patchedBuilder);
    when(delegate.beforeLoadTemplate(42L, originalLoad)).thenReturn(patchedLoad);
    when(delegate.beforeUpdateMetaData(42L, originalUpdate)).thenReturn(patchedUpdate);
    when(delegate.beforeUpdateGlobalProps(42L, originalProps)).thenReturn(patchedProps);
    when(delegate.beforeUpdateViewport(42L, 10, 20)).thenReturn(patchedSpecs);

    assertTrue(LynxHostScriptViewHooks.isEnabled());
    assertSame(patchedBuilder, LynxHostScriptViewHooks.beforeCreate(context, originalBuilder));
    assertSame(patchedLoad, LynxHostScriptViewHooks.beforeLoadTemplate(render, originalLoad));
    assertSame(patchedUpdate, LynxHostScriptViewHooks.beforeUpdateMetaData(render, originalUpdate));
    assertSame(
        patchedProps, LynxHostScriptViewHooks.beforeUpdateGlobalProps(render, originalProps));
    assertEquals(patchedSpecs, LynxHostScriptViewHooks.beforeUpdateViewport(render, 10, 20));

    LynxHostScriptViewHooks.register(view, render);
    LynxHostScriptViewHooks.unregister(render);
    LynxHostScriptViewHooks.clearViewUrl(42L);
    verify(delegate).registerView(view, 42L);
    verify(delegate).unregisterView(42L);
    verify(delegate).clearViewUrl(42L);
  }

  @Test
  public void registrationSkipsMissingOrUnavailableViewIdentity() {
    LynxHostScriptViewHooks.Delegate delegate = mock(LynxHostScriptViewHooks.Delegate.class);
    LynxHostScriptViewHooks.setDelegate(delegate);
    LynxView view = mock(LynxView.class);
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    when(render.getLogContextSnapshot()).thenReturn(LynxLogContext.UNAVAILABLE);

    LynxHostScriptViewHooks.register(view, null);
    LynxHostScriptViewHooks.register(view, render);
    LynxHostScriptViewHooks.unregister(null);
    LynxHostScriptViewHooks.unregister(render);

    verify(delegate, never()).registerView(any(), anyLong());
    verify(delegate, never()).unregisterView(anyLong());
    verify(delegate, never()).beforeUpdateViewport(anyLong(), anyInt(), anyInt());
  }

  @Test
  public void onMeasureAppliesViewportInterceptorSpecs() {
    LynxHostScriptViewHooks.Delegate delegate = mock(LynxHostScriptViewHooks.Delegate.class);
    LynxHostScriptViewHooks.setDelegate(delegate);

    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxView view = new LynxView(context);
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    ILynxUIRenderer uiRenderer = mock(ILynxUIRenderer.class);
    view.mLynxTemplateRender = render;
    when(render.getLogContextSnapshot()).thenReturn(new LynxLogContext(42L, 1L, 2L));
    when(render.lynxUIRenderer()).thenReturn(uiRenderer);
    when(delegate.isEnabled()).thenReturn(true);

    int hostWidth = MeasureSpec.makeMeasureSpec(1080, MeasureSpec.EXACTLY);
    int hostHeight = MeasureSpec.makeMeasureSpec(2400, MeasureSpec.EXACTLY);
    int patchedWidth = MeasureSpec.makeMeasureSpec(561, MeasureSpec.EXACTLY);
    int patchedHeight = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
    long packed = ((long) patchedWidth << 32) | (patchedHeight & 0xffffffffL);
    when(delegate.beforeUpdateViewport(42L, hostWidth, hostHeight)).thenReturn(packed);

    view.onMeasure(hostWidth, hostHeight);

    assertEquals(patchedWidth, view.getCurrentWidthMeasureSpec());
    assertEquals(patchedHeight, view.getCurrentHeightMeasureSpec());
    verify(delegate).beforeUpdateViewport(42L, hostWidth, hostHeight);
    verify(render).onMeasure(patchedWidth, patchedHeight);
  }
}
