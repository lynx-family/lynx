// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import android.renderscript.Matrix4f;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.behavior.ui.utils.TransformProps;

public interface IRendererHost {
  void setRenderer(Renderer renderer);
  Renderer getRenderer();
  View getView();
  Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign);

  default int getRendererHostWidth() {
    View view = getView();
    return view != null ? view.getWidth() : 0;
  }

  default int getRendererHostHeight() {
    View view = getView();
    return view != null ? view.getHeight() : 0;
  }

  default int getRendererHostScrollX() {
    View view = getView();
    return view != null ? view.getScrollX() : 0;
  }

  default int getRendererHostScrollY() {
    View view = getView();
    return view != null ? view.getScrollY() : 0;
  }

  default PointF convertPointInRendererHostToScreen(PointF point) {
    View view = getView();
    if (view == null) {
      return point;
    }
    return LynxUIHelper.convertPointInViewToScreen(view, point);
  }

  default void requestLayoutForRenderer() {
    View view = getView();
    if (view != null) {
      view.requestLayout();
    }
  }

  default void invalidateForRenderer() {
    View view = getView();
    if (view != null) {
      view.invalidate();
    }
  }

  default void setWillNotDrawForRenderer(boolean willNotDraw) {
    View view = getView();
    if (view != null) {
      view.setWillNotDraw(willNotDraw);
    }
  }

  default void setClipChildrenForRenderer(boolean clipChildren) {
    View view = getView();
    if (view instanceof ViewGroup) {
      ((ViewGroup) view).setClipChildren(clipChildren);
    }
  }

  default void applyRendererClipBounds(boolean needClip, Rect clipBounds) {
    View view = getView();
    if (view == null) {
      return;
    }
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR2) {
      view.setClipBounds(needClip ? clipBounds : null);
    }
  }

  default void applyRendererOpacity(float opacity) {
    View view = getView();
    if (view != null) {
      view.setAlpha(opacity);
    }
  }

  default void applyRendererTransform(float[] transform) {
    View view = getView();
    if (view == null) {
      return;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      view.setAnimationMatrix(Renderer.createTransformMatrix(transform));
      return;
    }
    TransformProps transformProps = new TransformProps();
    Matrix4f matrix4f = new Matrix4f(transform);
    TransformProps.matrix4fToTransformProps(matrix4f, transformProps);
    view.setTranslationX(transformProps.getTranslationX());
    view.setTranslationY(transformProps.getTranslationY());
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      view.setTranslationZ(transformProps.getTranslationZ());
    }
    view.setRotation(transformProps.getRotation());
    view.setRotationX(transformProps.getRotationX());
    view.setRotationY(transformProps.getRotationY());
    view.setScaleX(transformProps.getScaleX());
    view.setScaleY(transformProps.getScaleY());
  }
}
