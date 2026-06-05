// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.PointF;
import android.util.DisplayMetrics;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.behavior.ui.UIBody;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
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
  @Mock private BehaviorRegistry mockBehaviorRegistry;

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
    rendererContext =
        new PlatformRendererContext(mockBodyView, mockLynxContext, mockBehaviorRegistry);

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
  public void testGetTargetWidthHeight() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    FrameLayout view = new FrameLayout(context);
    view.layout(0, 0, 50, 60);
    IRendererHost host = createHost(view);
    rendererContext.mViewHolder.put(1, host);

    assertEquals(50, rendererContext.getTargetWidth(1));
    assertEquals(60, rendererContext.getTargetHeight(1));

    assertEquals(0, rendererContext.getTargetWidth(99));
    assertEquals(0, rendererContext.getTargetHeight(99));
  }

  @Test
  public void testGetRendererHostScrollOffset() {
    ViewGroup mockView = mock(ViewGroup.class);
    IRendererHost host = new TestRendererHost(mockView, null, 12, 34);
    rendererContext.mViewHolder.put(1, host);

    assertArrayEquals(new float[] {12, 34}, rendererContext.getRendererHostScrollOffset(1), 0);
    assertArrayEquals(new float[] {0, 0}, rendererContext.getRendererHostScrollOffset(99), 0);
  }

  @Test
  public void testIsRendererHostScrollable() {
    ViewGroup mockView = mock(ViewGroup.class);
    Renderer renderer = new Renderer(rendererContext, 1);
    LynxBaseUI uiHost = mock(LynxBaseUI.class);
    when(uiHost.isScrollable()).thenReturn(true);
    renderer.setUIHost(uiHost);
    rendererContext.mViewHolder.put(1, createHost(mockView, renderer));

    assertTrue(rendererContext.isRendererHostScrollable(1));
    assertFalse(rendererContext.isRendererHostScrollable(99));
  }

  @Test
  public void testConvertPointInViewToScreenDelegatesToRendererHost() {
    IRendererHost host = mock(IRendererHost.class);
    PointF point = new PointF(1, 2);
    PointF convertedPoint = new PointF(3, 4);
    when(host.convertPointInRendererHostToScreen(point)).thenReturn(convertedPoint);
    rendererContext.mViewHolder.put(1, host);

    assertSame(convertedPoint, rendererContext.convertPointInViewToScreen(1, point));
    verify(host).convertPointInRendererHostToScreen(point);
  }

  @Test
  public void testConvertPointInViewToScreenReturnsPointWhenHostMissing() {
    PointF point = new PointF(1, 2);

    assertSame(point, rendererContext.convertPointInViewToScreen(99, point));
  }

  @Test
  public void testDefaultRendererHostConvertPointReturnsPointWhenViewMissing() {
    IRendererHost host = new TestRendererHost(null);
    PointF point = new PointF(1, 2);

    assertSame(point, host.convertPointInRendererHostToScreen(point));
  }

  @Test
  public void testUpdatePlatformRendererFrame() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    FrameLayout view = spy(new FrameLayout(context));
    Renderer renderer = spy(new Renderer(rendererContext, 1));
    IRendererHost host = createHost(view, renderer);
    renderer.setRenderHost(host);
    rendererContext.mViewHolder.put(1, host);

    rendererContext.updatePlatformRendererFrame(1, true, 1, 2, 3, 4, 5, 6);

    verify(renderer).setLynxFrame(true, 1, 2, 1 + 3, 2 + 4, 5, 6);
    verify(view).requestLayout();
    verify(renderer).invalidate(Renderer.INVALIDATE_PARENT | Renderer.INVALIDATE_DISPLAY_LIST);
  }

  @Test
  public void testUpdatePlatformRendererAttributes() {
    ViewGroup mockView = mock(ViewGroup.class);
    Renderer renderer = spy(new Renderer(rendererContext, 1));
    IRendererHost host = createHost(mockView, renderer);
    renderer.setRenderHost(host);
    rendererContext.mViewHolder.put(1, host);

    PropBundle propBundle = mock(PropBundle.class);
    rendererContext.updatePlatformRendererAttributes(1, propBundle);

    verify(renderer).updateAttributes(propBundle);
  }

  @Test
  public void testUpdatePlatformRendererSubtreeProperties() {
    ViewGroup mockView = mock(ViewGroup.class);
    Renderer renderer = spy(new Renderer(rendererContext, 1));
    doNothing().when(renderer).applySubtreeProperties(any(ByteBuffer.class), anyInt());
    IRendererHost host = createHost(mockView, renderer);
    renderer.setRenderHost(host);
    rendererContext.mViewHolder.put(1, host);

    ByteBuffer buffer = ByteBuffer.allocate(68).order(ByteOrder.nativeOrder());
    rendererContext.updatePlatformRendererSubtreeProperties(1, buffer, 1);

    verify(renderer).applySubtreeProperties(buffer, 1);
  }

  @Test
  public void testUpdatePlatformExtraData() {
    ViewGroup mockView = mock(ViewGroup.class);
    Renderer renderer = spy(new Renderer(rendererContext, 1));
    IRendererHost host = createHost(mockView, renderer);
    renderer.setRenderHost(host);
    rendererContext.mViewHolder.put(1, host);

    Object extraData = new Object();
    rendererContext.updatePlatformExtraData(1, extraData);

    verify(renderer).updateExtraData(extraData);
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
    return new TestRendererHost(view);
  }

  private IRendererHost createHost(ViewGroup view, Renderer renderer) {
    return new TestRendererHost(view, renderer);
  }

  private static class TestRendererHost implements IRendererHost {
    private final ViewGroup view;
    private final int scrollX;
    private final int scrollY;
    private Renderer renderer;

    TestRendererHost(ViewGroup view) {
      this(view, null);
    }

    TestRendererHost(ViewGroup view, Renderer renderer) {
      this(view, renderer, 0, 0);
    }

    TestRendererHost(ViewGroup view, Renderer renderer, int scrollX, int scrollY) {
      this.view = view;
      this.renderer = renderer;
      this.scrollX = scrollX;
      this.scrollY = scrollY;
    }

    @Override
    public void setRenderer(Renderer renderer) {
      this.renderer = renderer;
    }

    @Override
    public Renderer getRenderer() {
      return renderer;
    }

    @Override
    public ViewGroup getView() {
      return view;
    }

    @Override
    public int getRendererHostScrollX() {
      return scrollX;
    }

    @Override
    public int getRendererHostScrollY() {
      return scrollY;
    }

    @Override
    public Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign) {
      return renderer != null ? renderer : new Renderer(platformRendererContext, sign);
    }
  }
}
