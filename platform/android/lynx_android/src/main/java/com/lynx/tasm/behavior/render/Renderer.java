// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static com.lynx.tasm.behavior.render.DisplayListApplier.SUBTREE_OP_OPACITY;
import static com.lynx.tasm.behavior.render.DisplayListApplier.SUBTREE_OP_TRANSFORM;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.PropBundle;

public class Renderer {
  private static final String TAG = "NativeUIRenderer.Renderer";
  public static final int INVALIDATE_PARENT = 1;
  public static final int INVALIDATE_DISPLAY_LIST = 1 << 1;
  public static final int REPAINT_TYPE_DRAW_ONLY = 1;
  public static final int REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW = 2;

  private static final int SUBTREE_PROPERTY_SIZE = 68; // 4 + 64 = 68 bytes
  private static final int TYPE_OFFSET = 0;
  private static final int DATA_OFFSET = 4;

  private final Rect mLynxFrame = new Rect();
  private final Point mRenderOffset = new Point();
  private final int mSign;
  private final PlatformRendererContext mPlatformRendererContext;
  private DisplayListApplier mDisplayListApplier = null;
  private final DisplayList mDisplayList = new DisplayList();
  private IRendererHost mRenderHost;
  private LynxBaseUI mUIHost;

  private int mRepaintType = REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW;

  public void setLynxFrame(boolean needClip, int l, int t, int r, int b, int dx, int dy) {
    mLynxFrame.set(l + dx, t + dy, r + dx, b + dy);
    mRenderOffset.set(dx, dy);
    if (mRenderHost == null) {
      return;
    }
    if (needClip) {
      mRenderHost.applyRendererClipBounds(
          true, new Rect(0, 0, mLynxFrame.width(), mLynxFrame.height()));
    } else {
      View self = mRenderHost.getView();
      if ((mUIHost == null || !mUIHost.isFlatten()) && self != null
          && self.getParent() instanceof ViewGroup) {
        ViewGroup parent = (ViewGroup) self.getParent();
        parent.setClipChildren(false);
        parent.setClipToPadding(false);
      }
      mRenderHost.applyRendererClipBounds(false, null);
    }
  }

  public Point getRenderOffset() {
    return mRenderOffset;
  }

  public Rect getLynxFrame() {
    return mLynxFrame;
  }

  public Renderer(@NonNull PlatformRendererContext platformRendererContext, int sign) {
    mPlatformRendererContext = platformRendererContext;
    mSign = sign;
  }

  void setRenderHost(IRendererHost renderHost) {
    mRenderHost = renderHost;
  }

  public IRendererHost getRendererHost() {
    return mRenderHost;
  }

  public com.lynx.tasm.behavior.LynxContext getLynxContext() {
    return mPlatformRendererContext.getLynxContext();
  }

  int getSign() {
    return mSign;
  }

  public LynxBaseUI getUIHost() {
    return mUIHost;
  }

  public void setUIHost(LynxBaseUI uiHost) {
    mUIHost = uiHost;
  }

  public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (mRenderHost == null) {
      return;
    }
    if (mUIHost != null && mUIHost.isFlatten()) {
      return;
    }
    View view = mRenderHost.getView();
    if (!(view instanceof ViewGroup)) {
      return;
    }
    ViewGroup viewGroup = (ViewGroup) view;
    for (int i = 0; i < viewGroup.getChildCount(); i++) {
      View child = viewGroup.getChildAt(i);
      if (child instanceof IRendererHost) {
        Renderer childRenderer = ((IRendererHost) child).getRenderer();
        if (childRenderer == null) {
          continue;
        }
        Rect childFrame = childRenderer.getLynxFrame();
        child.measure(
            View.MeasureSpec.makeMeasureSpec(childFrame.width(), View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(childFrame.height(), View.MeasureSpec.EXACTLY));
      }
    }
  }

  public void onLayout(boolean changed, int l, int t, int r, int b) {
    if (mRenderHost == null) {
      return;
    }
    if (mUIHost != null && mUIHost.isFlatten()) {
      return;
    }
    View view = mRenderHost.getView();
    if (!(view instanceof ViewGroup)) {
      return;
    }
    ViewGroup viewGroup = (ViewGroup) view;
    for (int i = 0; i < viewGroup.getChildCount(); i++) {
      View child = viewGroup.getChildAt(i);
      if (child instanceof IRendererHost) {
        Renderer childRenderer = ((IRendererHost) child).getRenderer();
        if (childRenderer == null) {
          continue;
        }
        Rect childFrame = childRenderer.getLynxFrame();
        child.layout(childFrame.left, childFrame.top, childFrame.right, childFrame.bottom);
      }
    }
  }

  public void onDraw(Canvas canvas) {
    if (mRepaintType == REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW) {
      mPlatformRendererContext.getDisplayList(mSign, mDisplayList);
    }
    if (mDisplayListApplier == null) {
      mDisplayListApplier =
          new DisplayListApplier(mDisplayList, mPlatformRendererContext, mRenderHost);
    } else {
      mDisplayListApplier.setDisplayList(mDisplayList);
    }
    mRepaintType = REPAINT_TYPE_DRAW_ONLY;
  }

  public void beforeDrawHost(Canvas canvas) {
    if (mDisplayListApplier == null) {
      return;
    }
    mDisplayListApplier.drawTillNextView(canvas);
  }

  public void beforeDrawChild(Canvas canvas, View child) {
    if (mDisplayListApplier == null) {
      canvas.save();
      return;
    }
    mDisplayListApplier.drawTillNextView(canvas);
    canvas.save();
    if (child instanceof ContainerRenderer) {
      canvas.translate(-((ContainerRenderer) child).getRenderer().getRenderOffset().x,
          -((ContainerRenderer) child).getRenderer().getRenderOffset().y);
    }
  }

  public void afterDrawChild(Canvas canvas, View child) {
    canvas.restore();
  }

  public void afterDispatchDraw(Canvas canvas) {
    if (mDisplayListApplier == null) {
      return;
    }
    mDisplayListApplier.drawTillNextView(canvas);
    mDisplayListApplier.reset();
  }

  public void invalidate(int invalidateMask) {
    if (getRendererHost() == null) {
      return;
    }
    mRenderHost.invalidateForRenderer();
    if ((invalidateMask & INVALIDATE_PARENT) != 0 && mRenderHost.getView() != null
        && mRenderHost.getView().getParent() instanceof View) {
      ((View) mRenderHost.getView().getParent()).invalidate();
    }
    if ((invalidateMask & INVALIDATE_DISPLAY_LIST) != 0) {
      mRepaintType = REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW;
    }
  }

  public void updateAttributes(PropBundle props) {}
  public void updateExtraData(Object extraData) {}
  public void onDestroy() {}

  public void applySubtreeProperties(java.nio.ByteBuffer buffer, int count) {
    IRendererHost rendererHost = getRendererHost();
    if (buffer == null || count <= 0 || rendererHost == null) {
      return;
    }

    // iterate all SubtreeProperty
    for (int i = 0; i < count; i++) {
      // caculate current position in the buffer
      int position = i * SUBTREE_PROPERTY_SIZE;
      buffer.position(position);

      // read type (int)
      int type = buffer.getInt();

      // read data according to the type
      if (type == SUBTREE_OP_TRANSFORM) {
        float[] transform = new float[16];
        for (int j = 0; j < 16; j++) {
          transform[j] = buffer.getFloat();
        }
        rendererHost.applyRendererTransform(transform);
      } else if (type == SUBTREE_OP_OPACITY) {
        float opacity = buffer.getFloat();
        rendererHost.applyRendererOpacity(opacity);
      }
    }
  }

  public static Matrix createTransformMatrix(float[] transform) {
    Matrix matrix = new Matrix();
    float[] values = new float[9];
    // C++ Matrix44 is column-major: [m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32,
    // m03, m13, m23, m33] Android Matrix.setValues expects row-major: [scaleX, skewX, transX,
    // skewY, scaleY, transY, persp0, persp1, persp2]
    values[0] = transform[0]; // m00 -> scaleX
    values[1] = transform[4]; // m01 -> skewX
    values[2] = transform[12]; // m03 -> transX
    values[3] = transform[1]; // m10 -> skewY
    values[4] = transform[5]; // m11 -> scaleY
    values[5] = transform[13]; // m13 -> transY
    values[6] = transform[3]; // m30 -> persp0
    values[7] = transform[7]; // m31 -> persp1
    values[8] = transform[15]; // m33 -> persp2
    matrix.setValues(values);
    return matrix;
  }
}
