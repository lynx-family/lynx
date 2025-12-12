// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.content.res.Resources;
import android.util.DisplayMetrics;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PlatformRendererContextTest {
  @Mock private LynxContext mockLynxContext;
  @Mock private Resources mockResources;
  @Mock private DisplayMetrics mockDisplayMetrics;
  @Mock private UIBody.UIBodyView mockBodyView;

  private PlatformRendererContext rendererContext;
  private AtomicReference<Renderer> rootRendererRef;

  @Before
  public void setUp() {
    MockitoAnnotations.initMocks(this);
    LynxEnv.inst().initNativeLibraries(new INativeLibraryLoader() {
      @Override
      public void loadLibrary(String libName) throws UnsatisfiedLinkError {
        System.loadLibrary(libName);
      }
    });
    when(mockLynxContext.getResources()).thenReturn(mockResources);
    when(mockResources.getDisplayMetrics()).thenReturn(mockDisplayMetrics);
    when(mockLynxContext.getScreenMetrics()).thenReturn(mockDisplayMetrics);
    mockDisplayMetrics.density = 2;
    rendererContext = new PlatformRendererContext(mockBodyView, mockLynxContext);

    rootRendererRef = new AtomicReference<>();
    when(mockBodyView.createRenderer(any(PlatformRendererContext.class), anyInt()))
        .thenAnswer(invocation -> {
          PlatformRendererContext context = invocation.getArgument(0);
          int sign = invocation.getArgument(1);
          return new Renderer(context, sign);
        });
    doAnswer(invocation -> {
      Renderer renderer = invocation.getArgument(0);
      rootRendererRef.set(renderer);
      return null;
    })
        .when(mockBodyView)
        .setRenderer(any(Renderer.class));
    when(mockBodyView.getRenderer()).thenAnswer(invocation -> rootRendererRef.get());
    when(mockBodyView.getView()).thenReturn(mockBodyView);
  }

  @Test
  public void testConstructorWithRootView() {
    assertNotNull(rendererContext);
    assertNotNull(rendererContext.getNativePtr());
    assertEquals(mockBodyView, rendererContext.mRootView.get());
  }

  @Test
  public void testSetRootView() {
    UIBody.UIBodyView newBodyView = mock(UIBody.UIBodyView.class);
    rendererContext.setRootView(newBodyView);
    assertEquals(newBodyView, rendererContext.mRootView.get());
  }

  @Test
  public void testCreatePlatformRenderer_PageType() {
    rendererContext.createPlatformRenderer(2, PlatformRendererContext.PlatformRendererType.kPage);
    assertNotNull(mockBodyView.getRenderer());
    assertEquals(2, mockBodyView.getRenderer().getSign());
    assertEquals(mockBodyView, rendererContext.mViewHolder.get(2));
  }

  @Test
  public void testInsertPlatformRenderer_AddAtEnd() {
    ViewGroup mockParentView = mock(ViewGroup.class);
    ViewGroup mockChildView = mock(ViewGroup.class);
    when(mockParentView.getChildCount()).thenReturn(2);
    IRendererHost parentHost = createHost(mockParentView);
    IRendererHost childHost = createHost(mockChildView);
    rendererContext.mViewHolder.put(1, parentHost);
    rendererContext.mViewHolder.put(2, childHost);

    rendererContext.insertPlatformRenderer(1, 2, -1);
    verify(mockParentView).addView(mockChildView);
  }

  @Test
  public void testInsertPlatformRenderer_AddAtIndex() {
    ViewGroup mockParentView = mock(ViewGroup.class);
    ViewGroup mockChildView = mock(ViewGroup.class);
    when(mockParentView.getChildCount()).thenReturn(5);
    IRendererHost parentHost = createHost(mockParentView);
    IRendererHost childHost = createHost(mockChildView);
    rendererContext.mViewHolder.put(1, parentHost);
    rendererContext.mViewHolder.put(2, childHost);

    rendererContext.insertPlatformRenderer(1, 2, 3);
    verify(mockParentView).addView(mockChildView, 3);
  }

  @Test
  public void testInvalidatePlatformRenderer() {
    ViewGroup mockView = mock(ViewGroup.class);
    IRendererHost host = createHost(mockView);
    rendererContext.mViewHolder.put(1, host);
    rendererContext.invalidatePlatformRenderer(1);
    verify(mockView).invalidate();
  }

  @Test
  public void testRemovePlatformRendererFromParent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    FrameLayout parent = new FrameLayout(context);
    FrameLayout child = new FrameLayout(context);
    parent.addView(child);
    IRendererHost childHost = createHost(child);
    rendererContext.mViewHolder.put(1, childHost);

    rendererContext.removePlatformRendererFromParent(1);
    assertEquals(0, parent.getChildCount());
    assertNull(child.getParent());
  }

  private IRendererHost createHost(ViewGroup view) {
    IRendererHost host = mock(IRendererHost.class);
    when(host.getView()).thenReturn(view);
    return host;
  }
}
