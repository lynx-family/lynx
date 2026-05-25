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
import android.os.Build;
import android.renderscript.Matrix4f;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.behavior.ui.utils.TransformProps;

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
  private LynxUI<ViewGroup> mUIHost;
  private TransformProps mTransformProps = null;

  private int mRepaintType = REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW;

  public void setLynxFrame(boolean needClip, int l, int t, int r, int b, int dx, int dy) {
    mLynxFrame.set(l + dx, t + dy, r + dx, b + dy);
    mRenderOffset.set(dx, dy);
    if (needClip) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
        mRenderHost.getView().setClipBounds(
            new Rect(0, 0, mLynxFrame.width(), mLynxFrame.height()));
      }
    } else {
      View self = mRenderHost.getView();
      ViewGroup parent = (ViewGroup) self.getParent();
      if (parent != null) {
        parent.setClipChildren(false);
        parent.setClipToPadding(false);
      }
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
        self.setClipBounds(null);
      }
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

  public LynxUI<ViewGroup> getUIHost() {
    return mUIHost;
  }

  public void setUIHost(LynxUI<ViewGroup> uiHost) {
    mUIHost = uiHost;
  }

  public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    View view = mRenderHost.getView();
    if (!(view instanceof ViewGroup)) {
      return;
    }
    ViewGroup viewGroup = (ViewGroup) view;
    for (int i = 0; i < viewGroup.getChildCount(); i++) {
      View child = viewGroup.getChildAt(i);
      if (child instanceof IRendererHost) {
        Rect childFrame = ((IRendererHost) child).getRenderer().getLynxFrame();
        child.measure(
            View.MeasureSpec.makeMeasureSpec(childFrame.width(), View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(childFrame.height(), View.MeasureSpec.EXACTLY));
      }
    }
  }

  public void onLayout(boolean changed, int l, int t, int r, int b) {
    View view = mRenderHost.getView();
    if (!(view instanceof ViewGroup)) {
      return;
    }
    ViewGroup viewGroup = (ViewGroup) view;
    for (int i = 0; i < viewGroup.getChildCount(); i++) {
      View child = viewGroup.getChildAt(i);
      if (child instanceof IRendererHost) {
        Rect childFrame = ((IRendererHost) child).getRenderer().getLynxFrame();
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
          new DisplayListApplier(mDisplayList, mPlatformRendererContext, mRenderHost.getView());
    } else {
      mDisplayListApplier.setDisplayList(mDisplayList);
    }
    mRepaintType = REPAINT_TYPE_DRAW_ONLY;
  }

  public void beforeDrawHost(Canvas canvas) {
    mDisplayListApplier.drawTillNextView(canvas);
  }

  public void beforeDrawChild(Canvas canvas, View child) {
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
    mDisplayListApplier.drawTillNextView(canvas);
    mDisplayListApplier.reset();
  }

  public void invalidate(int invalidateMask) {
    if (getRendererHost().getView() == null) {
      return;
    }
    mRenderHost.getView().invalidate();
    if ((invalidateMask & INVALIDATE_PARENT) != 0
        && mRenderHost.getView().getParent() instanceof View) {
      if (mRenderHost.getView().getParent() instanceof View) {
        ((View) mRenderHost.getView().getParent()).invalidate();
      }
    }
    if ((invalidateMask & INVALIDATE_DISPLAY_LIST) != 0) {
      mRepaintType = REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW;
    }
  }

  public void updateAttributes(PropBundle props) {}
  public void updateExtraData(Object extraData) {}
  public void onDestroy() {}

  public void applySubtreeProperties(java.nio.ByteBuffer buffer, int count) {
    if (buffer == null || count <= 0 || getRendererHost() == null
        || getRendererHost().getView() == null) {
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
        applyTransform(getRendererHost().getView(), transform);
      } else if (type == SUBTREE_OP_OPACITY) {
        float opacity = buffer.getFloat();
        applyOpacity(getRendererHost().getView(), opacity);
      }
    }
  }

  private void applyOpacity(View view, float opacity) {
    view.setAlpha(opacity);
  }

  private void applyTransform(View view, float[] transform) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      // Convert to Android Matrix (row-major 9 elements)
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
      view.setAnimationMatrix(matrix);
    } else {
      if (mTransformProps == null) {
        mTransformProps = new TransformProps();
      } else {
        mTransformProps.reset();
      }
      // Create Matrix4f from the 4x4 matrix data (column-major order)
      Matrix4f matrix4f = new Matrix4f(transform);
      TransformProps.matrix4fToTransformProps(matrix4f, mTransformProps);
      view.setTranslationX(mTransformProps.getTranslationX());
      view.setTranslationY(mTransformProps.getTranslationY());
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        view.setTranslationZ(mTransformProps.getTranslationZ());
      }
      view.setRotation(mTransformProps.getRotation());
      view.setRotationX(mTransformProps.getRotationX());
      view.setRotationY(mTransformProps.getRotationY());
      view.setScaleX(mTransformProps.getScaleX());
      view.setScaleY(mTransformProps.getScaleY());
    }
  }
}
