// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static android.os.Build.VERSION.SDK_INT;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RecordingCanvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.util.DisplayMetrics;
import androidx.annotation.CallSuper;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.rendernode.compat.RenderNodeCompat;
import com.lynx.tasm.rendernode.compat.RenderNodeFactory;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class LynxFlattenUI extends LynxBaseUI {
  private float mAlpha = 1.0f;
  private RenderNodeCompat mRenderNode;
  private boolean mIsValidate = false;

  // We need to support the dark mode?
  public static Method sSetUsageHint; // refers to LynxContext
  private RenderNodeCompat mBackgroundRenderNode;

  @Deprecated
  protected LynxFlattenUI(final Context context) {
    this((LynxContext) context);
  }

  public LynxFlattenUI(LynxContext context) {
    this(context, null);
  }

  public LynxFlattenUI(LynxContext context, Object param) {
    super(context, param);
    if (enableRenderNode()) {
      mRenderNode = RenderNodeFactory.getInstance().createRenderNodeCompat();
    }
  }

  @Override
  public boolean isFlatten() {
    return true;
  }

  @Override
  public void setSign(int sign, String tagName) {
    super.setSign(sign, tagName);
    if (mContext.getDefaultOverflowVisible()
        && (tagName.equals("view") || tagName.equals("component"))) {
      mOverflow = OVERFLOW_XY;
    }
  }

  @Override
  public int getInitialOverflowType() {
    return (mContext.getDefaultOverflowVisible()
               && (("view").equals(getTagName()) || ("component").equals(getTagName())))
        ? StyleConstants.OVERFLOW_VISIBLE
        : StyleConstants.OVERFLOW_HIDDEN;
  }

  @Override
  public void measure() {
    for (LynxBaseUI child : this.mChildren) {
      child.measure();
    }
  }

  public void layout(int x, int y, Rect bounds) {
    // set child's drawing x,y and bounds
    Rect childBounds = null;
    updateDrawingLayoutInfo(x, y, bounds);

    childBounds = new Rect(getLeft(), getTop(), getLeft() + getWidth(), getTop() + getHeight());
    final int overflow = getOverflow();
    final boolean isOverflowX = (overflow & OVERFLOW_X) != 0;
    final boolean isOverflowY = (overflow & OVERFLOW_Y) != 0;

    DisplayMetrics dm = getLynxContext().getScreenMetrics();
    if (isOverflowX && isOverflowY && bounds == null) {
      // self is overflow:visible and don't have bound
      // should not give child bound.
      childBounds = null;
    } else {
      if (isOverflowX) {
        if (bounds == null) {
          childBounds.left = -dm.widthPixels;
          childBounds.right = 2 * dm.widthPixels;
        } else {
          childBounds.left = bounds.left;
          childBounds.right = bounds.right;
        }
      } else if (bounds != null) {
        // overflowX: hidden and have bound from ancestor.
        childBounds.left = childBounds.left > bounds.left ? childBounds.left : bounds.left;
        childBounds.right = childBounds.right < bounds.right ? childBounds.right : bounds.right;
      }

      if (isOverflowY) {
        if (bounds == null) {
          childBounds.top = -dm.heightPixels;
          childBounds.bottom = 2 * dm.heightPixels;
        } else {
          childBounds.top = bounds.top;
          childBounds.bottom = bounds.bottom;
        }
      } else if (bounds != null) {
        // overflowY: hidden and have bound from ancestor.
        childBounds.top = childBounds.top > bounds.top ? childBounds.top : bounds.top;
        childBounds.bottom =
            childBounds.bottom < bounds.bottom ? childBounds.bottom : bounds.bottom;
      }
    }

    for (LynxBaseUI child : mChildren) {
      int childX = x + child.getOriginLeft();
      int childY = y + child.getOriginTop();
      if (!child.isFlatten()) {
        child.updateDrawingLayoutInfo(childX, childY, childBounds);
        ((LynxUI) child).layout();
      } else if (child.isFlatten()) {
        // LynxFlattenUI should calculate the real (x,y) and bounds.
        ((LynxFlattenUI) child).layout(childX, childY, childBounds);
      }
    }
  }

  @Override
  public void onDrawingPositionChanged() {
    invalidate();
  }

  @Override
  public void requestLayout() {
    mIsValidate = false;
    if (mDrawParent != null) {
      mDrawParent.requestLayout();
    }
  }

  @Override
  public void invalidate() {
    mIsValidate = false;
    if (mDrawParent != null) {
      mDrawParent.invalidate();
    }
  }

  @Override
  public float getTranslationX() {
    return 0;
  }

  @Override
  public float getTranslationY() {
    return 0;
  }

  @Override
  public float getTranslationZ() {
    return 0;
  }

  @Override
  public float getRealTimeTranslationZ() {
    return 0;
  }

  @LynxProp(name = PropsConstants.OPACITY, defaultFloat = 1.0f)
  public void setAlpha(float alpha) {
    mAlpha = alpha;
    invalidate();
  }

  /**
   * Similar to the BackgroundManager class. The former is set on the View, while flatten is
   * set in the TransFormProps class.
   * @param transform
   */
  @LynxProp(name = PropsConstants.TRANSFORM)
  public void setTransform(@Nullable ReadableArray transform) {
    super.setTransform(transform);
    invalidate();
  }

  final void innerDraw(Canvas canvas) {
    // TODO: 2020/8/9
    // Using hidden APIs carries high risks. Control whether to use RenderNode online; by
    // default, it is not used.
    // Independently validate in grayscale to check for crashes.
    // Before the official release, add try-catch protection. If a crash occurs, fallback to the old
    // logic.
    if (mRenderNode == null || !isHardwareDraw(canvas)) {
      draw(canvas);
      return;
    }
    boolean isValidate = mIsValidate;
    mIsValidate = true;
    if (!isValidate || !mRenderNode.hasDisplayList()) {
      updateRenderNode(mRenderNode);
    }
    if (!mRenderNode.hasDisplayList()) {
      return;
    }
    mRenderNode.drawRenderNode(canvas);
  }

  protected void updateRenderNode(RenderNodeCompat renderNode) {
    int w = getWidth();
    int h = getHeight();
    int left = getLeft();
    int top = getTop();
    int right = left + w;
    int bottom = top + h;
    if (getOverflow() != OVERFLOW_HIDDEN) {
      Rect clipRect = getClipBounds();
      right = clipRect.right + left;
      bottom = clipRect.bottom + top;
      left = clipRect.left + left;
      top = clipRect.top + top;
      w = right - left;
      h = bottom - top;
    }
    renderNode.setPosition(left, top, right, bottom);
    Canvas renderCanvas = renderNode.beginRecording(w, h);
    try {
      renderCanvas.translate(-left, -top);
      draw(renderCanvas);
    } finally {
      renderNode.endRecording(renderCanvas);
    }
  }

  private boolean isHardwareDraw(Canvas canvas) {
    return canvas.isHardwareAccelerated();
  }

  /**
   * you can disable render node by rewrite this method
   */
  protected boolean enableRenderNode() {
    return true;
  }

  @Override
  public void onAttach() {
    super.onAttach();
    invalidate();
  }

  public void draw(Canvas canvas) {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = getTagName() + ".flatten.draw";
      TraceEvent.beginSection(traceEvent);
    }
    if (mAlpha <= 0.0f) {
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
      return;
    }
    final int left = getLeft();
    final int top = getTop();

    int count = canvas.save();

    if ((left | top) != 0) {
      // fit drawing position
      canvas.translate(left, top);
    }

    if (mAlpha < 1.0f) {
      canvas.saveLayerAlpha(
          0, 0, getWidth(), getHeight(), (int) (mAlpha * 255), Canvas.ALL_SAVE_FLAG);
    }

    onDraw(canvas);
    canvas.restoreToCount(count);

    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @RequiresApi(api = Build.VERSION_CODES.Q)
  private RenderNodeCompat getDrawableRenderNode(Drawable drawable, RenderNodeCompat renderNode)
      throws InvocationTargetException, IllegalAccessException {
    if (renderNode == null) {
      renderNode = RenderNodeFactory.getInstance().createRenderNodeCompat();
      // Exception InvocationTargetException|IllegalAccessException throws here.
      sSetUsageHint.invoke(renderNode.getRenderNode(), 1);
      // RenderNode.USAGE_BACKGROUND == 1
    }

    final Rect bounds = drawable.getBounds();
    final int width = bounds.width();
    final int height = bounds.height();
    final Canvas canvas = renderNode.beginRecording(width, height);

    // Reverse left/top translation done by drawable canvas, which will
    // instead be applied by rendernode's LTRB bounds below. This way, the
    // drawable's bounds match with its rendernode bounds and its content
    // will lie within those bounds in the rendernode tree.
    canvas.translate(-bounds.left, -bounds.top);

    try {
      drawable.draw(canvas);
    } finally {
      renderNode.endRecording(canvas);
    }

    // Set up drawable properties that are view-independent.
    renderNode.setPosition(bounds.left, bounds.top, bounds.right, bounds.bottom);
    return renderNode;
  }

  @CallSuper
  public void onDraw(Canvas canvas) {
    final Drawable background = mLynxBackground.getDrawable();
    if (background == null) {
      return;
    }
    background.setBounds(0, 0, getWidth(), getHeight());
    if (mContext.getForceDarkAllowed() && sSetUsageHint != null && canvas.isHardwareAccelerated()) {
      try {
        mBackgroundRenderNode = getDrawableRenderNode(background, mBackgroundRenderNode);
        final RenderNodeCompat renderNode = mBackgroundRenderNode;

        if (renderNode.hasDisplayList()) {
          renderNode.drawRenderNode(canvas);
        } else {
          background.draw(canvas);
        }
      } catch (Exception e) {
        // Fallback to the original canvas
        background.draw(canvas);
      }
    } else {
      background.draw(canvas);
    }
  }

  public float getAlpha() {
    return mAlpha;
  }

  /**
   * Called by {@link LynxBaseUI#updateLayoutInfo}, FlattenUI drawing position (left, top) should
   * only changed in {@link #layout(int, int, Rect)}. Update the drawing position (left, top)
   * directly if the position from C++ (originLeft, originTop) is changed. Some customized ui will
   * use (left, top) in measure so the left, top value should update before layout. The offset from
   * (left, top) to (originLeft, originTop) is calculated during layout pass. Do not changed the
   * (left,top) value if the (originLeft, originTop) not changed to mitigate the UI display issue
   * when the next layout pass could not be arranged (e.g. requestLayout mark error, layout
   * operation triggered by recycler view).
   */
  @Override
  protected void setPosition(int left, int top) {
    if (getOriginTop() != top) {
      setOriginTop(top);
      setTop(top);
    }
    if (getOriginLeft() != left) {
      setOriginLeft(left);
      setLeft(left);
    }
  }
}
