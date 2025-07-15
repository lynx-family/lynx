// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.res.Resources;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import java.lang.reflect.Method;
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
    assertEquals(2, mockBodyView.mSign);
    assertEquals(mockBodyView, rendererContext.mViewHolder.get(2));
  }

  @Test
  public void testInsertPlatformRenderer_AddAtEnd() {
    ViewGroup mockParent = mock(ViewGroup.class);
    ViewGroup mockChild = mock(ViewGroup.class);
    when(mockParent.getChildCount()).thenReturn(2);
    rendererContext.mViewHolder.put(1, mockParent);
    rendererContext.mViewHolder.put(2, mockChild);

    rendererContext.insertPlatformRenderer(1, 2, -1);
    verify(mockParent).addView(mockChild);
  }

  @Test
  public void testInsertPlatformRenderer_AddAtIndex() {
    ViewGroup mockParent = mock(ViewGroup.class);
    ViewGroup mockChild = mock(ViewGroup.class);
    when(mockParent.getChildCount()).thenReturn(5);
    rendererContext.mViewHolder.put(1, mockParent);
    rendererContext.mViewHolder.put(2, mockChild);

    rendererContext.insertPlatformRenderer(1, 2, 3);
    verify(mockParent).addView(mockChild, 3);
  }

  @Test
  public void testInvalidatePlatformRenderer() {
    ViewGroup mockView = mock(ViewGroup.class);
    rendererContext.mViewHolder.put(1, mockView);
    rendererContext.invalidatePlatformRenderer(1);
    verify(mockView).invalidate();
  }

  @Test
  public void testRemovePlatformRendererFromParent() {
    try {
      Method field = View.class.getDeclaredMethod("getParent");
      field.setAccessible(true);
      ViewGroup parent = mock(ViewGroup.class);
      ViewGroup child = mock(ViewGroup.class);
      when(child.getParent()).thenReturn(parent);
      rendererContext.mViewHolder.put(1, child);
      rendererContext.removePlatformRendererFromParent(1);
      verify(parent).getParent();
    } catch (Exception e) {
    }
  }
}
