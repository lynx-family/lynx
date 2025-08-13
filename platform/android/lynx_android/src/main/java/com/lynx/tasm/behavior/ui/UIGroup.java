// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.image.FlattenUIImage;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.text.FlattenUIText;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.rendernode.compat.RenderNodeCompat;
import java.util.HashMap;
import java.util.Map;
import java.util.WeakHashMap;

public abstract class UIGroup<T extends ViewGroup>
    extends LynxUI<T> implements UIParent, IDrawChildHook {
  private static final String TAG = "UIGroup";

  private int mCurrentDrawIndex = 0;
  private LynxBaseUI mCurrentDrawUI = mDrawHead;
  private Rect mOverflowClipRect = new Rect();
  private static WeakHashMap<View, Integer> mZIndexHash = new WeakHashMap<>();
  private ViewGroupDrawingOrderHelper mDrawingOrderHelper;
  private boolean mIsInsertViewCalled = false;

  public boolean isInsertViewCalled() {
    return mIsInsertViewCalled;
  }

  public boolean enableAutoClipRadius() {
    return false;
  }

  public UIGroup(final LynxContext context) {
    this(context, null);
  }

  public UIGroup(final LynxContext context, Object param) {
    super(context, param);
  }

  @Override
  public void initialize() {
    super.initialize();
    mDrawingOrderHelper = new ViewGroupDrawingOrderHelper(getView());
    if (mView instanceof IDrawChildHookBinding) {
      ((IDrawChildHookBinding) mView).bindDrawChildHook(this);
    }
  }

  @Override
  public void markDetachWithViewRecursively(boolean detached) {
    if (detached) {
      mViewInfo = new ViewInfo(this, mView);
      if (mView instanceof IDrawChildHookBinding) {
        ((IDrawChildHookBinding) mView).bindDrawChildHook(mViewInfo);
      }
    }
    super.markDetachWithViewRecursively(detached);
  }

  // The following code shares structural similarities with IDrawChildHook implementations
  // Potential code reuse opportunities exist, but deferred to:
  // 1. Prevent coupling with existing logic
  // 2. Simplify code review process
  // Code optimization will be implemented via the following patch.
  @Override
  public void beforeProcessViewInfo(ViewInfo info) {
    super.beforeProcessViewInfo(info);
    if (mDrawingOrderHelper != null) {
      mDrawingOrderHelper.prepareChildDrawingOrder();
      info.setDrawingOrder(mDrawingOrderHelper.getDrawingOrderIndices());
    }
    info.setHasOverlappingRendering(hasOverlappingRendering());
  }

  @Override
  public void beforeDispatchProcessViewInfo(ViewInfo info) {
    super.beforeDispatchProcessViewInfo(info);
    mCurrentDrawUI = mDrawHead;
    mCurrentDrawIndex = 0;
    info.clearSubDrawInfo();
    boolean clipRadius = getClipToRadius()
        || (mContext.getDefaultOverflowVisible() && mOverflow == OVERFLOW_HIDDEN
            && enableAutoClipRadius());
    info.setClipToRadius(clipRadius);
    if (clipRadius) {
      BackgroundDrawable drawable =
          getLynxBackground() != null ? getLynxBackground().getDrawable() : null;
      Path path = drawable != null ? drawable.getInnerClipPathForBorderRadius() : null;
      boolean hasShear = getSkewX() != 0 || getSkewY() != 0;
      if (path != null) {
        info.setClipPathInBeforeDispatchDraw(path);
        info.setClipRectInBeforeDispatchDraw(null);
      } else if (hasShear) {
        info.setClipPathInBeforeDispatchDraw(null);
        info.setClipRectInBeforeDispatchDraw(getClipBounds());
      } else {
        info.setClipPathInBeforeDispatchDraw(null);
        info.setClipRectInBeforeDispatchDraw(null);
      }
    }
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2
        && getOverflow() != OVERFLOW_XY) {
      // setClipBounds can not be used prior to API 18, force clip here
      int w = getWidth(), h = getHeight();
      int x = 0, y = 0;
      DisplayMetrics dm = mContext.getScreenMetrics();
      if ((getOverflow() & OVERFLOW_X) != 0) {
        x -= dm.widthPixels;
        w += 2 * dm.widthPixels;
      }
      if ((getOverflow() & OVERFLOW_Y) != 0) {
        y -= dm.heightPixels;
        h += 2 * dm.heightPixels;
      }
      mOverflowClipRect.set(x, y, x + w, y + h);
      info.setOverflowClipRect(mOverflowClipRect);
    } else {
      info.setOverflowClipRect(null);
    }
  }

  @Override
  public void beforeProcessChildViewInfo(ViewInfo info, View child, long drawingTime) {
    super.beforeProcessChildViewInfo(info, child, drawingTime);
    for (LynxBaseUI ui = mCurrentDrawUI; ui != null; ui = ui.mNextDrawUI, mCurrentDrawIndex++) {
      if (!ui.isFlatten()) {
        if (((LynxUI) ui).getView() == child) {
          mCurrentDrawUI = ui.mNextDrawUI;
          ViewInfo.SubDrawInfo subInfo = new ViewInfo.SubDrawInfo(true, ui.getBound(), null, null,
              ((LynxUI<?>) ui).mViewInfo, ((LynxUI<?>) ui).getView());
          info.addSubDrawInfo(mCurrentDrawIndex, subInfo);
          subInfo.recordSubView(ui, child);
          mCurrentDrawIndex++;
          break;
        }
      } else if (ui.isFlatten()) {
        ViewInfo.SubDrawInfo subDrawInfo =
            new ViewInfo.SubDrawInfo(false, ui.getBound(), null, ui.getLynxBackground());
        subDrawInfo.setFlattenUIInfo(ui.getLeft(), ui.getTop(), ui.getWidth(), ui.getHeight(),
            ui.getPaddingLeft(), ui.getPaddingTop(), ui.getPaddingRight(), ui.getPaddingBottom(),
            ((LynxFlattenUI) ui).getAlpha());
        tryAddInfoForSubDraw(subDrawInfo, ui);
        info.addSubDrawInfo(mCurrentDrawIndex, subDrawInfo);
      }
    }
  }

  @Override
  public void afterDispatchProcessViewInfo(ViewInfo info) {
    super.afterDispatchProcessViewInfo(info);
    LynxBaseUI ui;
    for (ui = mCurrentDrawUI; ui != null; ui = ui.mNextDrawUI, mCurrentDrawIndex++) {
      if (ui.isFlatten() && !(ui instanceof UIShadowProxy)) {
        ViewInfo.SubDrawInfo subDrawInfo =
            new ViewInfo.SubDrawInfo(false, ui.getBound(), null, ui.getLynxBackground());
        subDrawInfo.setFlattenUIInfo(ui.getLeft(), ui.getTop(), ui.getWidth(), ui.getHeight(),
            ui.getPaddingLeft(), ui.getPaddingTop(), ui.getPaddingRight(), ui.getPaddingBottom(),
            ((LynxFlattenUI) ui).getAlpha());
        tryAddInfoForSubDraw(subDrawInfo, ui);
        info.addSubDrawInfo(mCurrentDrawIndex, subDrawInfo);
      }
    }
  }

  @Override
  public void processLayoutChildren() {
    performLayoutChildrenUI();
  }

  @Override
  public void processMeasureChildren() {
    performMeasureChildrenUI();
  }

  private void tryAddInfoForSubDraw(ViewInfo.SubDrawInfo subDrawInfo, LynxBaseUI ui) {
    if (ui instanceof FlattenUIImage) {
      FlattenUIImage image = (FlattenUIImage) ui;
      subDrawInfo.setImageManager(image.getLynxImageManagerForViewInfo());
    }
    // We need TextLayout, since we need to support font family, inline image when reuse LynxEngine.
    if (ui instanceof FlattenUIText) {
      FlattenUIText text = (FlattenUIText) ui;
      subDrawInfo.setTextLayout(text.getTextLayout());
      subDrawInfo.setDrawOffset(text.getDrawOffsetLeft(), text.getDrawOffsetTop());
    }
  }

  protected View getRealParentView() {
    return mView;
  }

  public void onInsertChild(LynxBaseUI child, int index) {
    child.setOffsetDescendantRectToLynxView(getOffsetDescendantRectToLynxView());
    mChildren.add(index, child);
    child.setParent(this);
  }

  public void insertChildWhenRebuildView(LynxBaseUI child) {
    if (!(child instanceof LynxUI)) {
      return;
    }

    LynxBaseUI ui = mDrawHead;
    int nonFlattenIndex = 0;

    while (ui != null && child != ui) {
      if (ui instanceof LynxUI) {
        nonFlattenIndex++;
      }
      ui = ui.mNextDrawUI;
    }

    View childView = ((LynxUI<?>) child).getView();
    if (childView.getParent() != null) {
      if (childView.getParent() == mView) {
        childView.requestLayout();
        return;
      }
      ((ViewGroup) childView.getParent()).removeView(childView);
    }

    mView.addView(childView, nonFlattenIndex);
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    // View will be added after insert drawList to ensure the order of view tree.
    onInsertChild(child, index);
    // Subclass not overwrite insertChild or will call insertChild
    mIsInsertViewCalled = true;
  }

  public void insertView(LynxUI child) {
    if (mContext != null && mContext.isFallbackProcess()
        && child.getView().getParent() == getView()) {
      child.getView().requestLayout();
      return;
    }

    int i = -1;
    for (LynxBaseUI ui = mDrawHead; ui != null; ui = ui.mNextDrawUI) {
      if (ui instanceof LynxUI) {
        ++i;
      }
      if (ui == child) {
        break;
      }
    }

    if (child.mView.getParent() != null && child.mView.getParent() instanceof ViewGroup) {
      ((ViewGroup) (child.mView.getParent())).removeView(child.mView);
      onRemoveChildUI(child);
    }

    mView.addView(child.mView, i);
    onAddChildUI(child, i);
  }

  public boolean onRemoveChild(LynxBaseUI child) {
    if (!mChildren.remove(child)) {
      return false;
    }
    child.setParent(null);
    return true;
  }

  public void removeChild(LynxBaseUI child) {
    if (onRemoveChild(child)) {
      removeView(child);
    }
  }

  public void removeView(LynxBaseUI child) {
    if (child instanceof LynxUI) {
      mView.removeView(((LynxUI) child).mView);
      if (child instanceof UIList) {
        mView.removeView(((UIList) child).getContainer());
      }
      onRemoveChildUI((LynxUI) child);
    } else {
      // FlattenUI removed should invalidate
      invalidate();
    }
  }

  public void removeAll() {
    for (LynxBaseUI ui = mDrawHead; ui != null; ui = ui.mNextDrawUI) {
      ui.setDrawParent(null);
    }
    mDrawHead = null;

    for (LynxBaseUI child : mChildren) {
      child.setParent(null);
    }
    mChildren.clear();
    if (mView != null) {
      mView.removeAllViews();
    }
  }

  public void measureChildren() {
    for (LynxBaseUI child : mChildren) {
      child.measure();
    }
  }

  public void layoutChildren() {
    for (int index = 0; index < mChildren.size(); index++) {
      LynxBaseUI child = mChildren.get(index);
      if (!needCustomLayout()) {
        if (!child.isFlatten()) {
          ((LynxUI) child).layout();
        } else {
          ((LynxFlattenUI) child).layout(child.getOriginLeft(), child.getOriginTop(), null);
        }
      } else if (child instanceof UIGroup) {
        ((UIGroup) child).layoutChildren();
      }
    }
  }

  @Override
  public void measure() {
    if (!mView.isLayoutRequested()) {
      return;
    }
    measureChildren();
    super.measure();
  }

  @Override
  public void layout() {
    if (!mView.isLayoutRequested()) {
      return;
    }
    super.layout();
    layoutChildren();
  }

  @Override
  public void beforeDispatchDraw(final Canvas canvas) {
    mCurrentDrawUI = mDrawHead;
    mCurrentDrawIndex = 0;
    boolean hasShear = getSkewX() != 0 || getSkewY() != 0;

    if (getClipToRadius()
        || (mContext.getDefaultOverflowVisible() && mOverflow == OVERFLOW_HIDDEN
            && enableAutoClipRadius())) {
      Drawable drawable = getLynxBackground() != null ? getLynxBackground().getDrawable() : null;
      if (drawable != null && drawable instanceof BackgroundDrawable) {
        Path path = ((BackgroundDrawable) drawable).getInnerClipPathForBorderRadius();
        if (path != null) {
          canvas.clipPath(path);
        } else if (hasShear) {
          // Shearing transformation should clip bounds manually.
          canvas.clipRect(getClipBounds());
        }
      }
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2
        && getOverflow() != OVERFLOW_XY) {
      // setClipBounds can not be used prior to API 18, force clip here
      int w = getWidth(), h = getHeight();
      int x = 0, y = 0;
      DisplayMetrics dm = mContext.getScreenMetrics();
      if ((getOverflow() & OVERFLOW_X) != 0) {
        x -= dm.widthPixels;
        w += 2 * dm.widthPixels;
      }
      if ((getOverflow() & OVERFLOW_Y) != 0) {
        y -= dm.heightPixels;
        h += 2 * dm.heightPixels;
      }
      mOverflowClipRect.set(x, y, x + w, y + h);
      canvas.clipRect(mOverflowClipRect);
    }
  }

  @Override
  public void afterDispatchDraw(final Canvas canvas) {
    LynxBaseUI ui;
    for (ui = mCurrentDrawUI; ui != null; ui = ui.mNextDrawUI) {
      if (ui.isFlatten() && !(ui instanceof UIShadowProxy)) {
        drawChild((LynxFlattenUI) ui, canvas);
      }
    }
  }

  @Override
  public void afterDraw(Canvas canvas) {}

  @Override
  public void beforeDraw(Canvas canvas) {
    if (getSkewX() != 0 || getSkewY() != 0) {
      canvas.skew(getSkewX(), getSkewY());
      // Put the anchor point back.
      // skew-x = tan(x) where x is the angle on x axis.
      // skew-y = tan(y) where y is the angle on y axis.
      // All points are convert by x' = x + y * tan(x), y' = y + x * tan(y). Move the anchor point
      // back. Please check the definition of shearing transformation for more details.
      canvas.translate(-mView.getPivotY() * getSkewX(), -mView.getPivotX() * getSkewY());
    }

    if (mClipPath != null) {
      Path path = mClipPath.getPath(getWidth(), getHeight());
      if (path != null) {
        canvas.clipPath(path);
      }
    }
  }

  private Rect drawFlattenUIBefore(final Canvas canvas, final View child, final long drawingTime) {
    Rect bound = null;
    for (LynxBaseUI ui = mCurrentDrawUI; ui != null; ui = ui.mNextDrawUI) {
      if (!ui.isFlatten()) {
        if (((LynxUI) ui).getView() == child) {
          bound = ui.getBound();
          mCurrentDrawUI = ui.mNextDrawUI;
          break;
        }
      } else if (ui.isFlatten()) {
        drawChild((LynxFlattenUI) ui, canvas);
      }
    }
    return bound;
  }

  @Override
  public Rect beforeDrawChild(final Canvas canvas, final View child, final long drawingTime) {
    Rect bound = drawFlattenUIBefore(canvas, child, drawingTime);
    return bound;
  }

  @Override
  public void afterDrawChild(final Canvas canvas, final View child, final long drawingTime) {}

  @Override
  public int getChildDrawingOrder(int childCount, int index) {
    if (mDrawingOrderHelper != null) {
      return mDrawingOrderHelper.getChildDrawingOrder(childCount, index);
    }
    return index;
  }

  @Override
  public boolean hasOverlappingRendering() {
    return hasOverlappingRenderingEnabled();
  }

  @Override
  public void performLayoutChildrenUI() {
    layoutChildren();
  }

  @Override
  public void performMeasureChildrenUI() {
    measureChildren();
  }

  protected void drawChild(LynxFlattenUI child, Canvas canvas) {
    Rect bound = child.getBound();
    canvas.save();
    if (bound != null) {
      canvas.clipRect(bound);
    }
    child.innerDraw(canvas);
    canvas.restore();
  }

  @Override
  public void destroy() {
    super.destroy();
    for (LynxBaseUI ui : mChildren) {
      ui.destroy();
    }
  }

  @Override
  public void onAttach() {
    super.onAttach();
    dispatchOnAttach();
  }

  @Override
  public void onDetach() {
    super.onDetach();
    dispatchOnDetach();
  }

  public void dispatchOnAttach() {
    for (LynxBaseUI ui : mChildren) {
      ui.onAttach();
    }
  }

  public void dispatchOnDetach() {
    for (LynxBaseUI ui : mChildren) {
      ui.onDetach();
    }
  }

  public int getIndex(LynxBaseUI child) {
    return mChildren.indexOf(child);
  }

  public int getChildCount() {
    return mChildren.size();
  }

  @Override
  public LynxBaseUI getChildAt(int index) {
    return mChildren.get(index);
  }

  public ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams childParams) {
    return null;
  }

  public boolean needCustomLayout() {
    return false;
  }

  public EventTarget findUIWithCustomLayout(float x, float y, UIGroup parent) {
    Map<View, LynxUI> children = new HashMap<>();
    for (int i = parent.getChildCount() - 1; i >= 0; i--) {
      LynxBaseUI child = parent.getChildAt(i);
      if (child instanceof UIShadowProxy) {
        child = ((UIShadowProxy) child).getChild();
      }
      if (child instanceof LynxUI) {
        children.put(((LynxUI) child).getView(), (LynxUI) child);
      } else {
        LLog.DTHROW(
            new RuntimeException("ui that need custom layout should not have flatten child!"));
      }
    }
    return findUIWithCustomLayoutByChildren(x, y, parent, children);
  }

  protected EventTarget findUIWithCustomLayoutByChildren(
      float x, float y, UIGroup parent, Map<View, LynxUI> children) {
    float[] eventCoords = new float[] {x, y};
    // eventCoords will be transformed to descendant's coordinate system
    LynxUI touchTarget =
        findTouchTargetOnViewChian(eventCoords, (ViewGroup) parent.getView(), children);

    if (touchTarget == null) {
      return parent;
    }

    if (touchTarget.needCustomLayout() && touchTarget instanceof UIGroup) {
      return ((UIGroup) touchTarget)
          .findUIWithCustomLayout(eventCoords[0], eventCoords[1], (UIGroup) touchTarget);
    }
    if (mContext.getEnableEventRefactor()) {
      return touchTarget.hitTest(eventCoords[0], eventCoords[1]);
    }
    return touchTarget.hitTest(
        eventCoords[0] + touchTarget.getScrollX(), eventCoords[1] + touchTarget.getScrollY());
  }

  private LynxUI findTouchTargetOnViewChian(
      float[] eventCoords, ViewGroup viewGroup, Map<View, LynxUI> relations) {
    LynxUI touchTarget = null;
    int childrenCount = viewGroup.getChildCount();
    for (int i = childrenCount - 1; i >= 0; i--) {
      View child = viewGroup.getChildAt(i);
      if (mContext.getEnableEventRefactor()) {
        float[] childPoint = new float[2];
        if (isTransformedTouchPointInView(eventCoords, viewGroup, child, childPoint)) {
          if (relations.containsKey(child)) {
            touchTarget = relations.get(child);
            eventCoords[0] = childPoint[0];
            eventCoords[1] = childPoint[1];
          } else if (child instanceof ViewGroup) {
            touchTarget = findTouchTargetOnViewChian(childPoint, (ViewGroup) child, relations);
            if (touchTarget != null) {
              eventCoords[0] = childPoint[0];
              eventCoords[1] = childPoint[1];
            }
          }
          if (touchTarget == null) {
            continue;
          }
          return touchTarget;
        }
        continue;
      }

      PointF childPoint = mTempPoint;
      if (isTransformedTouchPointInView(
              eventCoords[0], eventCoords[1], viewGroup, child, childPoint)) {
        float prex = eventCoords[0];
        float prey = eventCoords[1];
        eventCoords[0] = childPoint.x;
        eventCoords[1] = childPoint.y;
        if (relations.containsKey(child)) {
          touchTarget = relations.get(child);
        } else if (child instanceof ViewGroup) {
          touchTarget = findTouchTargetOnViewChian(eventCoords, (ViewGroup) child, relations);
        }
        if (touchTarget == null) {
          eventCoords[0] = prex;
          eventCoords[1] = prey;
          continue;
        }
        return touchTarget;
      }
    }
    return touchTarget;
  }

  private static final float[] mEventCoords = new float[2];
  private static final PointF mTempPoint = new PointF();
  private static final float[] mMatrixTransformCoords = new float[2];
  private static final Matrix mInverseMatrix = new Matrix();

  private boolean isTransformedTouchPointInView(
      float[] inPoint, View parent, View child, float[] outLocalPoint) {
    float[] point = getTargetPoint(
        inPoint[0], inPoint[1], parent.getScrollX(), parent.getScrollY(), child, child.getMatrix());
    outLocalPoint[0] = point[0];
    outLocalPoint[1] = point[1];
    if ((outLocalPoint[0] >= 0 && outLocalPoint[0] < (child.getRight() - child.getLeft()))
        && (outLocalPoint[1] >= 0 && outLocalPoint[1] < (child.getBottom() - child.getTop()))) {
      return true;
    }
    return false;
  }

  private boolean isTransformedTouchPointInView(
      float x, float y, ViewGroup parent, View child, PointF outLocalPoint) {
    float localX = x + parent.getScrollX() - child.getLeft();
    float localY = y + parent.getScrollY() - child.getTop();
    Matrix matrix = child.getMatrix();
    if (!matrix.isIdentity()) {
      float[] localXY = mMatrixTransformCoords;
      localXY[0] = localX;
      localXY[1] = localY;
      Matrix inverseMatrix = mInverseMatrix;
      matrix.invert(inverseMatrix);
      inverseMatrix.mapPoints(localXY);
      localX = localXY[0];
      localY = localXY[1];
    }
    if ((localX >= 0 && localX < (child.getRight() - child.getLeft()))
        && (localY >= 0 && localY < (child.getBottom() - child.getTop()))) {
      outLocalPoint.set(localX, localY);
      return true;
    }

    return false;
  }

  private void onAddChildUI(LynxUI child, int index) {
    if (!ENABLE_ZINDEX) {
      return;
    }
    mDrawingOrderHelper.handleAddView(child.getView());
    setChildrenDrawingOrderEnabledHelper(mDrawingOrderHelper.shouldEnableCustomDrawingOrder());
  }

  private void onRemoveChildUI(LynxUI child) {
    if (!ENABLE_ZINDEX) {
      return;
    }
    mDrawingOrderHelper.handleRemoveView(child.getView());
    setChildrenDrawingOrderEnabledHelper(mDrawingOrderHelper.shouldEnableCustomDrawingOrder());
  }

  public static void setViewZIndex(View view, int zIndex) {
    mZIndexHash.put(view, zIndex);
  }

  public static @Nullable Integer getViewZIndex(View view) {
    return mZIndexHash.get(view);
  }

  public void updateDrawingOrder() {
    mDrawingOrderHelper.update();
    setChildrenDrawingOrderEnabledHelper(mDrawingOrderHelper.shouldEnableCustomDrawingOrder());
    invalidate();
  }

  public View getAccessibilityHostView() {
    return mView;
  }

  private void setChildrenDrawingOrderEnabledHelper(boolean enable) {
    if (mView instanceof AndroidView) {
      ((AndroidView) mView).setChildrenDrawingOrderEnabled(enable);
    } else if (mView instanceof UIBody.UIBodyView) {
      ((UIBody.UIBodyView) mView).setChildrenDrawingOrderEnabled(enable);
    }
  }

  @Override
  public void setTranslationZ(float zValue) {
    super.setTranslationZ(zValue);
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP && zValue != getLastTranslateZ()) {
      setLastTranslateZ(zValue);
      setViewZIndex(mView, Math.round(zValue));
      UIParent parent = getParent();
      if (parent instanceof UIGroup) {
        ((UIGroup) parent).updateDrawingOrder();
      }
    }
  }
}
