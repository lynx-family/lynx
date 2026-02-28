// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.lynx.tasm.behavior.LayoutNodeManager;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class FrameShadowNodeTest {
  private FrameShadowNode frameShadowNode;
  private LayoutNodeManager layoutNodeManager;

  @Before
  public void setUp() {
    frameShadowNode = new FrameShadowNode();
    layoutNodeManager = mock(LayoutNodeManager.class);
    frameShadowNode.setLayoutNodeManager(layoutNodeManager);
  }

  @After
  public void tearDown() {
    frameShadowNode = null;
  }

  @Test
  public void testUpdateIntrinsicContentSize() {
    // Update size, should mark dirty
    frameShadowNode.updateIntrinsicContentSize(100, 200);
    verify(layoutNodeManager, times(1)).markDirty(anyInt());

    // Update with same size, should not mark dirty again
    frameShadowNode.updateIntrinsicContentSize(100, 200);
    verify(layoutNodeManager, times(1)).markDirty(anyInt());

    // Reset dirty state to simulate layout pass or fresh start for verification logic
    // However, LayoutNode.markDirty checks mIsDirty flag. If it is already true, it won't call
    // manager.markDirty. So we need to reset mIsDirty to false to test subsequent calls that expect
    // markDirty propagation.
    frameShadowNode.resetIsDirty();

    // Update width only
    frameShadowNode.updateIntrinsicContentSize(150, 200);
    verify(layoutNodeManager, times(2)).markDirty(anyInt());

    frameShadowNode.resetIsDirty();

    // Update height only
    frameShadowNode.updateIntrinsicContentSize(150, 250);
    verify(layoutNodeManager, times(3)).markDirty(anyInt());
  }

  @Test
  public void testMeasureUndefinedIntrinsicSize() {
    // Default intrinsic size is 0, 0
    MeasureParam param = new MeasureParam();
    param.mWidth = 300;
    param.mWidthMode = MeasureMode.EXACTLY;
    param.mHeight = 400;
    param.mHeightMode = MeasureMode.EXACTLY;

    MeasureResult result = frameShadowNode.measure(param, null);
    Assert.assertEquals("Width should be equal to param width when intrinsic size is not set", 300,
        result.getWidthResult(), 0.01f);
    Assert.assertEquals("Height should be equal to param height when intrinsic size is not set",
        400, result.getHeightResult(), 0.01f);
  }

  @Test
  public void testMeasureWithIntrinsicSize() {
    frameShadowNode.updateIntrinsicContentSize(100, 200);

    // MeasureMode.EXACTLY should override intrinsic size
    MeasureParam paramExactly = new MeasureParam();
    paramExactly.mWidth = 300;
    paramExactly.mWidthMode = MeasureMode.EXACTLY;
    paramExactly.mHeight = 400;
    paramExactly.mHeightMode = MeasureMode.EXACTLY;

    MeasureResult resultExactly = frameShadowNode.measure(paramExactly, null);
    Assert.assertEquals("Width should be from param when MeasureMode is EXACTLY", 300,
        resultExactly.getWidthResult(), 0.01f);
    Assert.assertEquals("Height should be from param when MeasureMode is EXACTLY", 400,
        resultExactly.getHeightResult(), 0.01f);

    // MeasureMode.AT_MOST should use intrinsic size
    MeasureParam paramAtMost = new MeasureParam();
    paramAtMost.mWidth = 300;
    paramAtMost.mWidthMode = MeasureMode.AT_MOST;
    paramAtMost.mHeight = 400;
    paramAtMost.mHeightMode = MeasureMode.AT_MOST;

    MeasureResult resultAtMost = frameShadowNode.measure(paramAtMost, null);
    Assert.assertEquals("Width should be intrinsic width when MeasureMode is AT_MOST", 100,
        resultAtMost.getWidthResult(), 0.01f);
    Assert.assertEquals("Height should be intrinsic height when MeasureMode is AT_MOST", 200,
        resultAtMost.getHeightResult(), 0.01f);

    // MeasureMode.UNDEFINED should use intrinsic size
    MeasureParam paramUndefined = new MeasureParam();
    paramUndefined.mWidth = 0;
    paramUndefined.mWidthMode = MeasureMode.UNDEFINED;
    paramUndefined.mHeight = 0;
    paramUndefined.mHeightMode = MeasureMode.UNDEFINED;

    MeasureResult resultUndefined = frameShadowNode.measure(paramUndefined, null);
    Assert.assertEquals("Width should be intrinsic width when MeasureMode is UNDEFINED", 100,
        resultUndefined.getWidthResult(), 0.01f);
    Assert.assertEquals("Height should be intrinsic height when MeasureMode is UNDEFINED", 200,
        resultUndefined.getHeightResult(), 0.01f);
  }
}
