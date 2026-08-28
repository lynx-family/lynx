// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.transfer;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.lynx.tasm.behavior.LayoutNodeManager;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

public class TransferShadowNodeTest {
  private LayoutNodeManager mLayoutNodeManager;
  private TransferShadowNode mShadowNode;

  @Before
  public void setUp() {
    mLayoutNodeManager = mock(LayoutNodeManager.class);
    mShadowNode = new TransferShadowNode();
    mShadowNode.setLayoutNodeManager(mLayoutNodeManager);
    mShadowNode.setSignature(10);
  }

  @Test
  public void attachNativePtrRegistersCustomMeasureFunction() {
    mShadowNode.attachNativePtr(1L);

    verify(mLayoutNodeManager).setMeasureFunc(10, mShadowNode);
  }

  @Test
  public void updateHostConstraintsMarksDirtyOnlyWhenConstraintsChange() {
    mShadowNode.updateHostConstraints(100, MeasureMode.EXACTLY, 200, MeasureMode.AT_MOST);
    verify(mLayoutNodeManager).markDirty(10);

    mShadowNode.updateHostConstraints(100, MeasureMode.EXACTLY, 200, MeasureMode.AT_MOST);
    verify(mLayoutNodeManager, times(1)).markDirty(10);

    mShadowNode.resetIsDirty();
    mShadowNode.updateHostConstraints(101, MeasureMode.EXACTLY, 200, MeasureMode.AT_MOST);
    verify(mLayoutNodeManager, times(2)).markDirty(10);

    mShadowNode.resetIsDirty();
    mShadowNode.updateHostConstraints(101, MeasureMode.EXACTLY, 201, MeasureMode.AT_MOST);
    verify(mLayoutNodeManager, times(3)).markDirty(10);

    mShadowNode.resetIsDirty();
    mShadowNode.updateHostConstraints(101, MeasureMode.EXACTLY, 201, MeasureMode.EXACTLY);
    verify(mLayoutNodeManager, times(4)).markDirty(10);

    mShadowNode.resetIsDirty();
    mShadowNode.updateHostConstraints(101, MeasureMode.AT_MOST, 201, MeasureMode.EXACTLY);
    verify(mLayoutNodeManager, times(5)).markDirty(10);
  }

  @Test
  public void measureWithoutHostConstraintsSkipsChildren() {
    NativeLayoutNodeRef nativeChild = mock(NativeLayoutNodeRef.class);
    mShadowNode.addChildAt(nativeChild, 0);
    mShadowNode.attachNativePtr(1L);

    float[] result = mShadowNode.measure(
        300, MeasureMode.EXACTLY.intValue(), 400, MeasureMode.AT_MOST.intValue(), true);

    assertEquals(0, result[0], 0.01f);
    assertEquals(0, result[1], 0.01f);
    verify(nativeChild, never()).measureNativeNode(any(MeasureContext.class), any());
  }

  @Test
  public void measureUsesHostConstraintsForNativeChildren() {
    NativeLayoutNodeRef nativeChild = mock(NativeLayoutNodeRef.class);
    mShadowNode.addChildAt(nativeChild, 0);
    mShadowNode.addChildAt(new ShadowNode(), 1);
    mShadowNode.attachNativePtr(1L);
    mShadowNode.updateHostConstraints(320, MeasureMode.EXACTLY, 180, MeasureMode.AT_MOST);

    float[] result = mShadowNode.measure(
        500, MeasureMode.AT_MOST.intValue(), 600, MeasureMode.UNDEFINED.intValue(), true);

    ArgumentCaptor<MeasureParam> paramCaptor = ArgumentCaptor.forClass(MeasureParam.class);
    verify(nativeChild).measureNativeNode(any(MeasureContext.class), paramCaptor.capture());
    MeasureParam childParam = paramCaptor.getValue();
    assertEquals(320, childParam.mWidth, 0.01f);
    assertEquals(MeasureMode.EXACTLY, childParam.mWidthMode);
    assertEquals(180, childParam.mHeight, 0.01f);
    assertEquals(MeasureMode.AT_MOST, childParam.mHeightMode);
    assertEquals(0, result[0], 0.01f);
    assertEquals(0, result[1], 0.01f);
  }

  @Test
  public void undefinedHostConstraintFallsBackToParentConstraint() {
    NativeLayoutNodeRef nativeChild = mock(NativeLayoutNodeRef.class);
    mShadowNode.addChildAt(nativeChild, 0);
    mShadowNode.attachNativePtr(1L);
    mShadowNode.updateHostConstraints(0, MeasureMode.UNDEFINED, 180, MeasureMode.EXACTLY);

    mShadowNode.measure(
        500, MeasureMode.AT_MOST.intValue(), 600, MeasureMode.UNDEFINED.intValue(), false);

    ArgumentCaptor<MeasureParam> paramCaptor = ArgumentCaptor.forClass(MeasureParam.class);
    verify(nativeChild).measureNativeNode(any(MeasureContext.class), paramCaptor.capture());
    MeasureParam childParam = paramCaptor.getValue();
    assertEquals(500, childParam.mWidth, 0.01f);
    assertEquals(MeasureMode.AT_MOST, childParam.mWidthMode);
    assertEquals(180, childParam.mHeight, 0.01f);
    assertEquals(MeasureMode.EXACTLY, childParam.mHeightMode);
  }

  @Test
  public void alignOnlyAlignsNativeChildrenAtOrigin() {
    NativeLayoutNodeRef nativeChild = mock(NativeLayoutNodeRef.class);
    mShadowNode.addChildAt(nativeChild, 0);
    mShadowNode.addChildAt(new ShadowNode(), 1);
    mShadowNode.attachNativePtr(1L);

    mShadowNode.align();

    ArgumentCaptor<AlignParam> paramCaptor = ArgumentCaptor.forClass(AlignParam.class);
    verify(nativeChild).alignNativeNode(any(AlignContext.class), paramCaptor.capture());
    assertEquals(0, paramCaptor.getValue().getLeftOffset(), 0.01f);
    assertEquals(0, paramCaptor.getValue().getTopOffset(), 0.01f);
    verify(mLayoutNodeManager, never()).alignNativeNode(anyInt(), anyFloat(), anyFloat());
  }
}
