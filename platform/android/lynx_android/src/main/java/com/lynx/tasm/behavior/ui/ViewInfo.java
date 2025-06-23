// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.Rect;
import android.text.Layout;
import android.view.View;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.IProcessViewInfoHook;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.shapes.BasicShape;
import com.lynx.tasm.behavior.ui.utils.MaskDrawable;
import com.lynx.tasm.rendernode.compat.RenderNodeCompat;
import java.util.ArrayList;

// ViewInfo is a critical class for enabling the reuse of the Lynx Engine. It stores essential
// rendering metadata (e.g., layout coordinates, clipping boundaries). After decoupling the
// Platform View from LynxUI (the cross-platform UI abstraction layer), ViewInfo acts as a
// platform-agnostic data layer to coordinate drawing operations.
public class ViewInfo implements IDrawChildHook {
  public ViewInfo(IProcessViewInfoHook hook, View view) {
    mProcessHook = hook;
    mView = view;
  }

  public void detachFromUI() {
    mProcessHook = null;
  }

  IProcessViewInfoHook mProcessHook;
  View mView;

  // beforeDraw
  int mWidth;
  int mHeight;
  float mSkewX;
  float mSkewY;
  BasicShape mClipPath;
  int[] mOrder;
  boolean mHasOverlappingRendering;

  public void detachWithUI() {}

  public void setWidth(int width) {
    mWidth = width;
  }

  public void setHeight(int height) {
    mHeight = height;
  }

  public void setSkewX(float x) {
    mSkewX = x;
  }

  public void setSkewY(float y) {
    mSkewY = y;
  }

  public void setClipPath(BasicShape path) {
    mClipPath = path;
  }

  public void setDrawingOrder(int[] order) {
    mOrder = order;
  }

  public void setHasOverlappingRendering(boolean enable) {
    mHasOverlappingRendering = enable;
  }

  // beforeDispatchDraw
  boolean clipToRadius;
  Path mClipPathInBeforeDispatchDraw;
  Rect mClipRectInBeforeDispatchDraw;
  Rect mOverflowClipRect;

  public void setClipToRadius(boolean enable) {
    clipToRadius = enable;
  }

  public void setClipPathInBeforeDispatchDraw(Path path) {
    mClipPathInBeforeDispatchDraw = path;
  }

  public void setClipRectInBeforeDispatchDraw(Rect rect) {
    mClipRectInBeforeDispatchDraw = rect;
  }

  public void setOverflowClipRect(Rect rect) {
    mOverflowClipRect = rect;
  }

  // beforeDrawChild
  public static class SubDrawInfo {
    boolean mIsView;
    Rect mBound;
    RenderNodeCompat mRenderNode;
    LynxImageManager mLynxImageManager = null;
    Layout mTextLayout = null;
    int mLeft = 0;
    int mTop = 0;

    public SubDrawInfo(boolean isView, Rect bound, RenderNodeCompat renderNode) {
      mIsView = isView;
      mBound = bound;
      mRenderNode = renderNode;
    }

    public void setImageManager(LynxImageManager imageManager) {
      mLynxImageManager = imageManager;
    }

    public void setTextLayout(Layout textLayout) {
      mTextLayout = textLayout;
    }

    public void setDrawPosition(int left, int top) {
      mLeft = left;
      mTop = top;
    }
  }

  int mCurrentDrawIndex = 0;
  // index same with the UI index in parent's children
  ArrayList<SubDrawInfo> mSubDrawInfoArray = new ArrayList<>();

  public void addSubDrawInfo(int index, SubDrawInfo info) {
    if (index < 0) {
      return;
    }
    int currentSize = mSubDrawInfoArray.size();
    if (index > currentSize) {
      // 填充 null 到中间位置
      int nullsToAdd = index - currentSize;
      for (int i = 0; i < nullsToAdd; i++) {
        mSubDrawInfoArray.add(null);
      }
      mSubDrawInfoArray.add(info);
    } else {
      mSubDrawInfoArray.add(index, info);
    }
  }

  public void clearSubDrawInfo() {
    mSubDrawInfoArray.clear();
  }

  // afterDrawChild
  // Nothing now
  // afterDraw
  int mBoundsWidth;
  int mBoundsHeight;
  MaskDrawable mMaskDrawable;

  private LynxImageManager mImageManagerUsedInBeforeDraw;

  public void setBoundsWidth(int width) {
    mBoundsWidth = width;
  }

  public void setBoundsHeight(int height) {
    mBoundsHeight = height;
  }

  public void setMaskDrawable(MaskDrawable drawable) {
    mMaskDrawable = drawable;
  }

  public void setImageManagerUsedInBeforeDraw(LynxImageManager manager) {
    mImageManagerUsedInBeforeDraw = manager;
  }

  @Override
  public void beforeDraw(Canvas canvas) {
    if (mProcessHook != null) {
      mProcessHook.beforeProcessViewInfo(this);
    }

    if (mImageManagerUsedInBeforeDraw != null) {
      mImageManagerUsedInBeforeDraw.onDraw(canvas);
    }

    if (mSkewX != 0 || mSkewY != 0) {
      canvas.skew(mSkewX, mSkewY);
      // Put the anchor point back.
      // skew-x = tan(x) where x is the angle on x axis.
      // skew-y = tan(y) where y is the angle on y axis.
      // All points are convert by x' = x + y * tan(x), y' = y + x * tan(y). Move the anchor point
      // back. Please check the definition of shearing transformation for more details.
      canvas.translate(-mView.getPivotY() * mSkewX, -mView.getPivotX() * mSkewY);
    }
    if (mClipPath != null) {
      Path path = mClipPath.getPath(mWidth, mHeight);
      if (path != null) {
        canvas.clipPath(path);
      }
    }
  }

  @Override
  public void beforeDispatchDraw(Canvas canvas) {
    if (mProcessHook != null) {
      mProcessHook.beforeDispatchProcessViewInfo(this);
    }
    mCurrentDrawIndex = 0;
    if (clipToRadius) {
      if (mClipPathInBeforeDispatchDraw != null) {
        canvas.clipPath(mClipPathInBeforeDispatchDraw);
      } else if (mClipRectInBeforeDispatchDraw != null) {
        canvas.clipRect(mClipRectInBeforeDispatchDraw);
      }
    }
    if (mOverflowClipRect != null) {
      canvas.clipRect(mOverflowClipRect);
    }
  }

  @Override
  public Rect beforeDrawChild(Canvas canvas, View child, long drawingTime) {
    if (mProcessHook != null) {
      mProcessHook.beforeProcessChildViewInfo(this, child, drawingTime);
    }
    Rect bound = null;
    for (; mCurrentDrawIndex < mSubDrawInfoArray.size(); ++mCurrentDrawIndex) {
      SubDrawInfo info = mSubDrawInfoArray.get(mCurrentDrawIndex);
      if (info.mIsView) {
        bound = info.mBound;
        mCurrentDrawIndex++;
      } else {
        drawWithSubDrawInfo(info, canvas);
      }
    }
    return bound;
  }

  private void drawWithSubDrawInfo(SubDrawInfo info, Canvas canvas) {
    Rect childBound = info.mBound;
    canvas.save();

    if (childBound != null) {
      canvas.clipRect(childBound);
    }

    if (info.mRenderNode != null) {
      info.mRenderNode.drawRenderNode(canvas);
    }

    if (info.mLeft != 0 || info.mTop != 0) {
      canvas.translate(info.mLeft, info.mTop);
    }

    if (info.mTextLayout != null) {
      info.mTextLayout.draw(canvas);
    }

    if (info.mLynxImageManager != null) {
      info.mLynxImageManager.onDraw(canvas);
    }

    canvas.restore();
  }

  @Override
  public void afterDrawChild(Canvas canvas, View child, long drawingTime) {
    if (mProcessHook != null) {
      mProcessHook.afterProcessChildViewInfo(this, child, drawingTime);
    }
  }

  @Override
  public void afterDispatchDraw(Canvas canvas) {
    if (mProcessHook != null) {
      mProcessHook.afterDispatchProcessViewInfo(this);
    }
    for (; mCurrentDrawIndex < mSubDrawInfoArray.size(); ++mCurrentDrawIndex) {
      SubDrawInfo info = mSubDrawInfoArray.get(mCurrentDrawIndex);
      drawWithSubDrawInfo(info, canvas);
    }
  }

  @Override
  public void afterDraw(Canvas canvas) {
    if (mProcessHook != null) {
      mProcessHook.afterProcessViewInfo(this);
    }
    if (mMaskDrawable != null) {
      mMaskDrawable.setBounds(0, 0, mBoundsWidth, mBoundsHeight);
      mMaskDrawable.draw(canvas);
    }
  }

  @Override
  public int getChildDrawingOrder(int childCount, int index) {
    if (mOrder == null || index >= mOrder.length) {
      return index;
    }
    return mOrder[index];
  }

  @Override
  public boolean hasOverlappingRendering() {
    return mHasOverlappingRendering;
  }
  @Override
  public void performLayoutChildrenUI() {
    if (mProcessHook != null) {
      mProcessHook.processLayoutChildren();
    }
  }

  @Override
  public void performMeasureChildrenUI() {
    if (mProcessHook != null) {
      mProcessHook.processMeasureChildren();
    }
  }

  public void invalidate() {
    if (mView == null) {
      return;
    }
    mView.invalidate();
  }
}
