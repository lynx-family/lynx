// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.graphics.Rect;
import com.lynx.tasm.behavior.render.LayerRenderContext;
import com.lynx.tasm.behavior.render.NativeDisplayListBuilder;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import java.nio.ByteBuffer;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

public class FragmentLayerUIDrawTest {
  private static final int PARENT_SIGN = 1;
  private static final int FLATTEN_SIGN = 2;
  private static final int CHILD_SIGN = 3;

  @Mock private LayerRenderContext mLayerRenderContext;
  @Mock private Canvas mCanvas;

  private LynxContext mContext;
  private LynxUIOwner.FragmentLayerUI mChild;
  private LynxFlattenUI mFlatten;
  private NativeDisplayListBuilder mParentDisplayList;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    mContext = TestingUtils.getLynxContext();
    com.lynx.tasm.LynxEnv.inst().init(
        (android.app.Application) androidx.test.platform.app.InstrumentationRegistry
            .getInstrumentation()
            .getTargetContext()
            .getApplicationContext(),
        null, null, null, null);
    NativeDisplayListBuilder.ensureRegistered();
    mChild = new LynxUIOwner.FragmentLayerUI(mContext);
    mChild.setSign(CHILD_SIGN, "view");
    mChild.attachFragmentLayer(CHILD_SIGN, mLayerRenderContext);
    mChild.setFragmentLayerFrame(false, 0, 0, 10, 10, 7, 9);
    mChild.setBound(new Rect(0, 0, 10, 10));
    mFlatten = new LynxFlattenUI(mContext);
    mFlatten.setSign(FLATTEN_SIGN, "view");
    mFlatten.attachFragmentLayer(FLATTEN_SIGN, mLayerRenderContext);
    mFlatten.setNextDrawUI(mChild);

    mParentDisplayList = new NativeDisplayListBuilder().drawView(CHILD_SIGN, 0f, 0f);
    when(mLayerRenderContext.getDisplayListItemsBuffer(PARENT_SIGN))
        .thenReturn(mParentDisplayList.toItemsBuffer());
    when(mLayerRenderContext.getDisplayListDataBuffer(PARENT_SIGN))
        .thenReturn(ByteBuffer.allocateDirect(0));
    when(mLayerRenderContext.getFragmentLayer(CHILD_SIGN)).thenReturn(mChild);
  }

  @After
  public void tearDown() {
    mParentDisplayList.close();
  }

  private void attachParent(UIGroup<?> parent) {
    parent.setSign(PARENT_SIGN, "view");
    parent.attachFragmentLayer(PARENT_SIGN, mLayerRenderContext);
    parent.setDrawHead(mFlatten);
  }

  @Test
  public void syntheticLayerDrawSkipsFlattenUI() {
    LynxUIOwner.FragmentLayerUI parent = new LynxUIOwner.FragmentLayerUI(mContext);
    attachParent(parent);
    parent.attachFragmentLayer(PARENT_SIGN, mLayerRenderContext, true);
    parent.setDrawHead(null);

    parent.beforeDispatchDraw(mCanvas);

    assertNull(parent.beforeDrawChild(mCanvas, mChild.getView(), 0L));
    parent.afterDrawChild(mCanvas, mChild.getView(), 0L);
    parent.afterDispatchDraw(mCanvas);

    verify(mCanvas).translate(-7, -9);
    verify(mLayerRenderContext, never()).getDisplayListItemsBuffer(FLATTEN_SIGN);
    verify(mLayerRenderContext, never()).getDisplayListDataBuffer(FLATTEN_SIGN);
  }

  @Test
  public void optedInComponentDrawsDisplayListWithoutLegacyDrawLinks() {
    UIView parent = new UIView(mContext);
    parent.setSign(PARENT_SIGN, "layer-content");
    parent.attachFragmentLayer(PARENT_SIGN, mLayerRenderContext, true);

    parent.beforeDispatchDraw(mCanvas);
    assertNull(parent.beforeDrawChild(mCanvas, mChild.getView(), 0L));
    parent.afterDrawChild(mCanvas, mChild.getView(), 0L);
    parent.afterDispatchDraw(mCanvas);

    verify(mCanvas).translate(-7, -9);
    verify(mLayerRenderContext, never()).getDisplayListItemsBuffer(FLATTEN_SIGN);
  }

  @Test
  public void optedInComponentRetainsCompatibleFlattenDrawing() {
    UIView parent = new UIView(mContext);
    parent.setSign(PARENT_SIGN, "layer-content");
    parent.attachFragmentLayer(PARENT_SIGN, mLayerRenderContext, true);
    parent.setDrawHead(mFlatten);

    parent.beforeDispatchDraw(mCanvas);
    assertNotNull(parent.beforeDrawChild(mCanvas, mChild.getView(), 0L));
    parent.afterDrawChild(mCanvas, mChild.getView(), 0L);
    parent.afterDispatchDraw(mCanvas);

    verify(mLayerRenderContext).getDisplayListItemsBuffer(FLATTEN_SIGN);
  }

  @Test
  public void platformExtendedUIGroupKeepsFlattenUIDrawing() {
    UIView parent = new UIView(mContext);
    attachParent(parent);

    parent.beforeDispatchDraw(mCanvas);

    assertNotNull(parent.beforeDrawChild(mCanvas, mChild.getView(), 0L));
    parent.afterDrawChild(mCanvas, mChild.getView(), 0L);
    parent.afterDispatchDraw(mCanvas);

    verify(mCanvas).translate(-7, -9);
    verify(mLayerRenderContext).getDisplayListItemsBuffer(FLATTEN_SIGN);
    verify(mLayerRenderContext).getDisplayListDataBuffer(FLATTEN_SIGN);
  }
}
