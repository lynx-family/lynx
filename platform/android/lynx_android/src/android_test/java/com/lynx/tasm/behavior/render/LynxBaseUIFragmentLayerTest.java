// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.testing.base.TestingUtils;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class LynxBaseUIFragmentLayerTest {
  private static final int TEST_SIGN = 17;

  @Mock private LayerRenderContext mLayerRenderContext;
  @Mock private Canvas mCanvas;

  private TestLayerUI mUI;
  private final ArrayList<NativeDisplayListBuilder> mDisplayLists = new ArrayList<>();

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    LynxContext context = TestingUtils.getLynxContext();
    com.lynx.tasm.LynxEnv.inst().init(
        (android.app.Application) androidx.test.platform.app.InstrumentationRegistry
            .getInstrumentation()
            .getTargetContext()
            .getApplicationContext(),
        null, null, null, null);
    NativeDisplayListBuilder.ensureRegistered();
    mUI = new TestLayerUI(context);
    mUI.setSign(TEST_SIGN, "view");
  }

  @After
  public void tearDown() {
    for (NativeDisplayListBuilder displayList : mDisplayLists) {
      displayList.close();
    }
    mDisplayLists.clear();
  }

  @Test
  public void attachAndDetachOwnFragmentLayerState() {
    mUI.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);

    assertTrue(mUI.isFragmentLayer());
    assertFalse(mUI.getView().willNotDraw());

    mUI.detachFragmentLayer();

    assertFalse(mUI.isFragmentLayer());
    assertTrue(mUI.getFragmentLayerFrame().isEmpty());
  }

  @Test
  public void frameAndClipAreAppliedByUI() {
    mUI.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);

    mUI.setFragmentLayerFrame(true, 10, 20, 110, 80, 2, 3);

    assertEquals(new Rect(12, 23, 112, 83), mUI.getFragmentLayerFrame());
    assertEquals(new Rect(0, 0, 100, 60), mUI.getView().getClipBounds());
  }

  @Test
  public void displayListUsesLayerSignAndIsFetchedAgainOnlyAfterInvalidation() {
    int layerSign = TEST_SIGN + 1;
    ByteBuffer items = ByteBuffer.allocateDirect(0);
    ByteBuffer data = ByteBuffer.allocateDirect(0);
    when(mLayerRenderContext.getDisplayListItemsBuffer(layerSign)).thenReturn(items);
    when(mLayerRenderContext.getDisplayListDataBuffer(layerSign)).thenReturn(data);
    mUI.attachFragmentLayer(layerSign, mLayerRenderContext);

    mUI.prepareFragmentLayerDisplayList(mCanvas);
    mUI.finishFragmentLayerDisplayList(mCanvas);
    mUI.prepareFragmentLayerDisplayList(mCanvas);
    verify(mLayerRenderContext, times(1)).getDisplayListItemsBuffer(layerSign);
    verify(mLayerRenderContext, times(1)).getDisplayListDataBuffer(layerSign);

    mUI.invalidateFragmentLayer(
        com.lynx.tasm.behavior.ui.LynxBaseUI.FRAGMENT_LAYER_INVALIDATE_DISPLAY_LIST);
    mUI.prepareFragmentLayerDisplayList(mCanvas);
    verify(mLayerRenderContext, times(2)).getDisplayListItemsBuffer(layerSign);
    verify(mLayerRenderContext, times(2)).getDisplayListDataBuffer(layerSign);
  }

  @Test
  public void unavailableDisplayListIsFetchedAgainOnNextDraw() {
    ByteBuffer items = ByteBuffer.allocateDirect(0);
    ByteBuffer data = ByteBuffer.allocateDirect(0);
    when(mLayerRenderContext.getDisplayListItemsBuffer(TEST_SIGN)).thenReturn(null, items);
    when(mLayerRenderContext.getDisplayListDataBuffer(TEST_SIGN)).thenReturn(null, data);
    mUI.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);

    mUI.prepareFragmentLayerDisplayList(mCanvas);
    mUI.prepareFragmentLayerDisplayList(mCanvas);
    mUI.prepareFragmentLayerDisplayList(mCanvas);

    verify(mLayerRenderContext, times(2)).getDisplayListItemsBuffer(TEST_SIGN);
    verify(mLayerRenderContext, times(2)).getDisplayListDataBuffer(TEST_SIGN);
  }

  @Test
  public void subtreeOpacityIsAppliedDirectlyToUI() {
    mUI.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);
    ByteBuffer buffer = ByteBuffer.allocate(68).order(ByteOrder.nativeOrder());
    buffer.putInt(DisplayListApplier.SUBTREE_OP_OPACITY);
    buffer.putFloat(0.35f);
    buffer.position(0);

    mUI.applyFragmentLayerSubtreeProperties(buffer, 1);

    assertEquals(0.35f, mUI.getView().getAlpha(), 0.0001f);
  }

  @Test
  public void groupDrawsParentDisplayListBeforeFlattenChildLayer() {
    int childSign = TEST_SIGN + 1;
    TestLayerGroup group = new TestLayerGroup(TestingUtils.getLynxContext());
    group.setSign(TEST_SIGN, "view");
    group.setWidth(100);
    group.setHeight(100);
    group.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);

    LynxFlattenUI child = new LynxFlattenUI(TestingUtils.getLynxContext());
    child.setSign(childSign, "view");
    child.setWidth(50);
    child.setHeight(50);
    child.attachFragmentLayer(childSign, mLayerRenderContext);
    group.setDrawHead(child);
    child.setDrawParent(group);

    when(mLayerRenderContext.getDisplayListItemsBuffer(TEST_SIGN))
        .thenReturn(createFillDisplayList(Color.YELLOW, childSign));
    when(mLayerRenderContext.getDisplayListItemsBuffer(childSign))
        .thenReturn(createFillDisplayList(Color.RED, null));
    List<Integer> colors = new ArrayList<>();
    doAnswer(invocation -> {
      colors.add(invocation.<Paint>getArgument(1).getColor());
      return null;
    })
        .when(mCanvas)
        .drawRect(any(RectF.class), any(Paint.class));

    group.beforeDispatchDraw(mCanvas);
    group.afterDispatchDraw(mCanvas);

    verify(mCanvas, times(2)).drawRect(any(RectF.class), any(Paint.class));
    assertEquals(Color.YELLOW, colors.get(0).intValue());
    assertEquals(Color.RED, colors.get(1).intValue());
  }

  @Test
  public void fragmentLayerGroupOnlyLaysOutChildrenWhenRequested() {
    TestLayerGroup group = new TestLayerGroup(TestingUtils.getLynxContext());
    group.setSign(TEST_SIGN, "view");
    group.setWidth(100);
    group.setHeight(100);
    group.attachFragmentLayer(TEST_SIGN, mLayerRenderContext);
    group.getView().measure(View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY));
    group.getView().layout(0, 0, 100, 100);
    assertFalse(group.getView().isLayoutRequested());

    TestLayerUI child = new TestLayerUI(TestingUtils.getLynxContext());
    child.setSign(TEST_SIGN + 1, "view");
    child.setLeft(10);
    child.setTop(20);
    child.setWidth(30);
    child.setHeight(40);
    group.onInsertChild(child, 0);

    group.layout();
    assertEquals(0, child.getView().getLeft());
    assertEquals(0, child.getView().getTop());
    assertEquals(0, child.getView().getRight());
    assertEquals(0, child.getView().getBottom());

    group.requestLayout();
    group.layout();
    assertEquals(10, child.getView().getLeft());
    assertEquals(20, child.getView().getTop());
    assertEquals(40, child.getView().getRight());
    assertEquals(60, child.getView().getBottom());
  }

  private ByteBuffer createFillDisplayList(int color, Integer childSign) {
    NativeDisplayListBuilder displayList = new NativeDisplayListBuilder();
    mDisplayLists.add(displayList);
    displayList.begin(0, PlatformRendererContext.PlatformRendererType.kView, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .fill(color, 0);
    if (childSign != null) {
      displayList.drawView(childSign, 0f, 0f);
    }
    displayList.end();
    return displayList.toItemsBuffer();
  }

  static class TestLayerUI extends LynxUI<View> {
    TestLayerUI(LynxContext context) {
      super(context);
    }

    @Override
    protected View createView(Context context) {
      return new View(InstrumentationRegistry.getInstrumentation().getTargetContext());
    }
  }

  static class TestLayerGroup extends UIGroup<FrameLayout> {
    TestLayerGroup(LynxContext context) {
      super(context);
    }

    @Override
    protected FrameLayout createView(Context context) {
      return new FrameLayout(InstrumentationRegistry.getInstrumentation().getTargetContext());
    }
  }
}
