// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Canvas;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.PropBundle;

public class Renderer {
  public static final int INVALIDATE_PARENT = 1;
  public static final int INVALIDATE_DISPLAY_LIST = 1 << 1;
  public static final int REPAINT_TYPE_DRAW_ONLY = 1;
  public static final int REPAINT_TYPE_GET_DISPLAY_LIST_AND_DRAW = 2;

  private final Rect mLynxFrame = new Rect();
  private final Point mRenderOffset = new Point();
  private final int mSign;
  private final PlatformRendererContext mPlatformRendererContext;
  private DisplayListApplier mDisplayListApplier = null;
  private final DisplayList mDisplayList = new DisplayList();
  private IRendererHost mRenderHost;

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

  public void onLayout(boolean changed, int l, int t, int r, int b) {
    ViewGroup view = mRenderHost.getView();
    for (int i = 0; i < view.getChildCount(); i++) {
      View child = view.getChildAt(i);
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
}
