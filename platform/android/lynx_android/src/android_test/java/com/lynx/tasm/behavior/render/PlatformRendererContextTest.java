// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.View;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.tasm.performance.PerformanceController;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ReadOnlyBufferException;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PlatformRendererContextTest {
  private static final int IMAGE_MODE_ASPECT_FILL = 2;
  @Mock private LynxContext mLynxContext;
  @Mock private Resources mResources;
  @Mock private UIBody.UIBodyView mBodyView;
  @Mock private BehaviorRegistry mBehaviorRegistry;
  @Mock private LynxUIOwner mUIOwner;

  private DisplayMetrics mDisplayMetrics;
  private PlatformRendererContext mRendererContext;

  @Before
  public void setUp() {
    LynxEnv.inst().initNativeLibraries(new INativeLibraryLoader() {
      @Override
      public void loadLibrary(String libName) throws UnsatisfiedLinkError {
        System.loadLibrary(libName);
      }
    });
    mDisplayMetrics = new DisplayMetrics();
    mDisplayMetrics.density = 2;
    when(mLynxContext.getResources()).thenReturn(mResources);
    when(mResources.getDisplayMetrics()).thenReturn(mDisplayMetrics);
    when(mLynxContext.getScreenMetrics()).thenReturn(mDisplayMetrics);
    when(mLynxContext.getLynxUIOwner()).thenReturn(mUIOwner);
    mRendererContext = new PlatformRendererContext(mBodyView, mLynxContext, mBehaviorRegistry);
  }

  @After
  public void tearDown() {
    if (mRendererContext != null) {
      mRendererContext.destroy();
    }
  }

  @Test
  public void constructorKeepsRootViewAndCreatesNativeContext() {
    assertNotNull(mRendererContext);
    assertTrue(mRendererContext.getNativePtr() != 0);
    assertSame(mBodyView, mRendererContext.mRootView.get());
  }

  @Test
  public void displayListBufferIsReadOnly() {
    ByteBuffer storage = ByteBuffer.allocateDirect(Integer.BYTES);
    storage.order(ByteOrder.nativeOrder()).putInt(0, 42);

    ByteBuffer buffer = PlatformRendererContext.makeReadOnlyDisplayListBuffer(storage);

    assertTrue(buffer.isDirect());
    assertTrue(buffer.isReadOnly());
    assertEquals(42, buffer.order(ByteOrder.nativeOrder()).getInt(0));
    try {
      buffer.put(0, (byte) 0);
      fail("Display list buffers must be read-only");
    } catch (ReadOnlyBufferException expected) {
      // Expected.
    }
  }

  @Test
  public void standardLayerCreationUsesUIOwner() {
    mRendererContext.createPlatformRenderer(7, PlatformRendererContext.PlatformRendererType.kView);

    verify(mUIOwner).createFragmentLayer(7, mRendererContext);
  }

  @Test
  public void extendedLayerUsesUIOwnerCreatedUI() {
    TestLayerUI ui = new TestLayerUI(mLynxContext);
    when(mUIOwner.attachFragmentLayer(9, mRendererContext)).thenReturn(ui);
    when(mBehaviorRegistry.get("custom")).thenReturn(new Behavior("custom"));

    mRendererContext.createPlatformExtendedRenderer(9, "custom", null);

    verify(mUIOwner).createView(9, "custom", null, null, null, false, 9, null);
    verify(mUIOwner).attachFragmentLayer(9, mRendererContext);
  }

  @Test
  public void insertAndRemoveAlwaysUseUIOwner() {
    TestLayerUI parent = new TestLayerUI(mLynxContext);
    TestLayerUI child = new TestLayerUI(mLynxContext);
    when(mUIOwner.getNode(1)).thenReturn(parent);
    when(mUIOwner.getNode(2)).thenReturn(child);

    mRendererContext.insertPlatformRenderer(1, 2, 0, false);
    verify(mUIOwner).insert(1, 2, 0);

    child.setParent(parent);
    mRendererContext.removePlatformRendererFromParent(1, 2, false);
    verify(mUIOwner).remove(1, 2);
  }

  @Test
  public void duplicateInsertDoesNotMutateOwnerTwice() {
    TestLayerUI parent = new TestLayerUI(mLynxContext);
    TestLayerUI child = new TestLayerUI(mLynxContext);
    parent.getChildren().add(child);
    child.setParent(parent);
    when(mUIOwner.getNode(1)).thenReturn(parent);
    when(mUIOwner.getNode(2)).thenReturn(child);

    mRendererContext.insertPlatformRenderer(1, 2, 0, true);

    verify(mUIOwner, never()).insert(anyInt(), anyInt(), anyInt());
  }

  @Test
  public void updateFrameUpdatesUIAndOwnerLayout() {
    TestLayerUI ui = new TestLayerUI(mLynxContext);
    ui.attachFragmentLayer(3, mRendererContext);
    when(mUIOwner.getNode(3)).thenReturn(ui);
    when(mUIOwner.getFragmentLayer(3)).thenReturn(ui);

    mRendererContext.updatePlatformRendererFrame(
        3, true, 10, 20, 100, 60, 2, 3, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);

    assertEquals(new Rect(12, 23, 112, 83), ui.getFragmentLayerFrame());
    verify(mUIOwner).updateLayout(
        3, 12, 23, 100, 60, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, null, null, 0, 3);
  }

  @Test
  public void updateAttributesUsesBehaviorFlattenCapability() {
    TestLayerUI ui = new TestLayerUI(mLynxContext);
    ui.setSign(4, "custom");
    ui.attachFragmentLayer(4, mRendererContext);
    when(mUIOwner.getNode(4)).thenReturn(ui);
    when(mUIOwner.getFragmentLayer(4)).thenReturn(ui);
    when(mBehaviorRegistry.get("custom")).thenReturn(new Behavior("custom", true));
    PropBundle props = mock(PropBundle.class);

    mRendererContext.updatePlatformRendererAttributes(4, props, true);

    verify(mUIOwner).updateProperties(eq(4), eq(true), isNull(), isNull(), isNull());
    verify(mUIOwner).updateFragmentLayer(4, ui, mRendererContext);
    assertTrue(ui.isFragmentLayer());
  }

  @Test
  public void destroyDetachesLayerAndUsesUIOwner() {
    mRendererContext.destroyPlatformRenderer(5);

    verify(mUIOwner).destroyFragmentLayer(5);
  }

  @Test
  public void meaningfulPaintingPropertiesComeFromOwnedUI() {
    TestLayerUI ui = new TestLayerUI(mLynxContext);
    ui.getView().setVisibility(View.INVISIBLE);
    ui.getView().setAlpha(0.4f);
    ui.getView().setScaleX(1.5f);
    ui.getView().setScaleY(0.6f);
    when(mUIOwner.getFragmentLayer(6)).thenReturn(ui);

    assertEquals(View.INVISIBLE, mRendererContext.getMeaningfulPaintingAreaVisibleStatus(6));
    assertEquals(0.4f, mRendererContext.getMeaningfulPaintingAreaAlpha(6), 0.f);
    assertEquals(1.5f, mRendererContext.getMeaningfulPaintingAreaScaleX(6), 0.f);
    assertEquals(0.6f, mRendererContext.getMeaningfulPaintingAreaScaleY(6), 0.f);
  }

  @Test
  public void testSetNeedMarkPaintEndTiming() {
    PerformanceController performanceController = mock(PerformanceController.class);
    when(mLynxContext.getPerfController()).thenReturn(performanceController);

    mRendererContext.setNeedMarkPaintEndTiming("pipeline-id");

    verify(performanceController).setNeedMarkPaintEndTiming("pipeline-id");
  }

  @Test
  public void testCreateImageInitializesImageManagerWithMode() throws Exception {
    ReadableMapBuffer paintInfo = mock(ReadableMapBuffer.class);
    when(paintInfo.getInt(1)).thenReturn(IMAGE_MODE_ASPECT_FILL);
    when(paintInfo.getString(2, null)).thenReturn(null);
    when(paintInfo.getBoolean(3, false)).thenReturn(true);
    when(paintInfo.getString(5, null)).thenReturn("#ff0000");
    when(paintInfo.getString(6, null)).thenReturn("1px 2px 3px 4px");
    when(paintInfo.getDouble(7, 1.0)).thenReturn(2.0);
    when(paintInfo.getBoolean(8, false)).thenReturn(true);
    when(paintInfo.getBoolean(9, true)).thenReturn(false);
    when(paintInfo.getInt(10, 0)).thenReturn(3);

    mRendererContext.createImage(7, null, paintInfo, 100, 60, 0, 11, false);

    ArgumentCaptor<LynxImageManager> imageManagerCaptor =
        ArgumentCaptor.forClass(LynxImageManager.class);
    verify(mBodyView).registerImageAccordingToNodeIndex(eq(11), imageManagerCaptor.capture());

    Field modeField = LynxImageManager.class.getDeclaredField("mMode");
    modeField.setAccessible(true);
    LynxImageManager imageManager = imageManagerCaptor.getValue();
    assertSame(ScalingUtils.ScaleType.CENTER_CROP, modeField.get(imageManager));
    assertEquals(true, getField(imageManager, "mAutoSize"));
    assertEquals("#ff0000", getField(imageManager, "mTintColor"));
    assertEquals("1px 2px 3px 4px", getField(imageManager, "mCapInsets"));
    assertEquals("2.0", getField(imageManager, "mCapInsetsScale"));
    assertEquals(true, getField(imageManager, "mSkipRedirection"));
    assertEquals(false, getField(imageManager, "mAutoPlay"));
    assertEquals(3, getField(imageManager, "mLoopCount"));
  }

  private static Object getField(Object target, String name) throws Exception {
    java.lang.reflect.Field field = target.getClass().getDeclaredField(name);
    field.setAccessible(true);
    return field.get(target);
  }

  @Test
  public void focusTransitionsUseOwnedUI() {
    LynxBaseUI first = mock(LynxBaseUI.class);
    LynxBaseUI second = mock(LynxBaseUI.class);
    when(mUIOwner.getNode(1)).thenReturn(first);
    when(mUIOwner.getNode(2)).thenReturn(second);
    when(first.isFocusable()).thenReturn(true);
    when(second.isFocusable()).thenReturn(true);
    mRendererContext.updatePlatformFocus(1, 1);
    mRendererContext.updatePlatformFocus(2, 2);
    org.mockito.InOrder order = org.mockito.Mockito.inOrder(first, second);
    order.verify(first).onFocusChanged(true, false);
    order.verify(second).onFocusChanged(true, true);
    order.verify(first).onFocusChanged(false, true);
  }

  static class TestLayerUI extends LynxUI<View> {
    TestLayerUI(LynxContext context) {
      super(context);
    }

    @Override
    protected View createView(Context context) {
      Context viewContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
      return new View(viewContext);
    }

    void setParent(LynxBaseUI parent) {
      mParent = parent;
    }
  }
}
