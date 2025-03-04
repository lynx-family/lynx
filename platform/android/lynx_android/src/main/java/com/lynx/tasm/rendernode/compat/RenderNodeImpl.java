// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.rendernode.compat;

import android.graphics.Canvas;
import android.graphics.RecordingCanvas;
import android.graphics.RenderNode;

public class RenderNodeImpl extends RenderNodeCompat {
  private RenderNode renderNode;

  @Override
  public void init() {
    renderNode = new RenderNode("");
  }

  @Override
  public void drawRenderNode(Canvas canvas) {
    ((RecordingCanvas) canvas).drawRenderNode(renderNode);
  }

  @Override
  public void drawRenderNode(Canvas renderCanvas, Object renderNode) {
    renderCanvas.drawRenderNode(((RenderNode) renderNode));
  }

  @Override
  public void setPosition(int left, int top, int right, int bottom) {
    renderNode.setPosition(left, top, right, bottom);
  }

  @Override
  public RecordingCanvas beginRecording(int width, int height) {
    return renderNode.beginRecording();
  }

  @Override
  public void endRecording(Canvas canvas) {
    renderNode.endRecording();
  }

  @Override
  public Object getRenderNode() {
    return renderNode;
  }

  @Override
  public boolean hasDisplayList() {
    return renderNode.hasDisplayList();
  }
}
