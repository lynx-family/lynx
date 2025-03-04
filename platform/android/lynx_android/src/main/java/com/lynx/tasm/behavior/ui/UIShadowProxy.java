// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static com.lynx.tasm.behavior.ui.ShadowData.parseShadow;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.Shader;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.animation.layout.LayoutAnimationManager;
import com.lynx.tasm.animation.transition.TransitionAnimationManager;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import com.lynx.tasm.behavior.ui.utils.TransformProps;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.utils.FloatUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class UIShadowProxy extends UIGroup<UIShadowProxy.ShadowView> {
  private static final String TAG = "UIShadowProxy";
  private LynxBaseUI mChild;
  private List<ShadowData> mBoxShadowList;
  private Outline mOutline;
  private Boolean mTransformToUpdate;
  private TransformProps mTransform;
  private Matrix mMatrix = new Matrix();
  private boolean mNeedUpdateShadow = false;
  public boolean isNeedUpdateShadow() {
    return mNeedUpdateShadow;
  }
  public void setNeedUpdateShadow(boolean needUpdateShadow) {
    mNeedUpdateShadow = needUpdateShadow;
  }

  public UIShadowProxy(LynxContext context, LynxBaseUI child) {
    super(context);
    mOverflow = OVERFLOW_XY;
    mChild = child;
    mChild.setParent(this);
    setSign(child.getSign(), child.getTagName());
    initialize();

    // When the custom ui needs to handle the insertion of child view itself, we use flag
    // mIsInsertViewCalled to avoid inserting child view in LynxUIOwner#insertIntoDrawList(). But if
    // the custom ui has the box-shadow prop, we can not invoke super.insertChild(child, 0) to avoid
    // flag mIsInsertViewCalled being set to true.
    onInsertChild(child, 0);
    super.insertDrawList(null, mChild);
    super.insertView((LynxUI) mChild);
    updateTransform();
  }

  // When enable new flatten, UIShadowProxy will replace its child, and UIShadowProxy overrides the
  // getChildren function. When exec UIShadowProxy's getChildren function, the result will be
  // child's children rather than the child. In this case, when exec
  // LynxUIOwner.findLynxUIByIdSelector function, it can't find UIShadowProxy's child since
  // UIShadowProxy's getChildren function return child's children. To avoid this bug, when enable
  // new flatten, override UIShadowProxy's getTagName & getIdSelector function. Detail and test case
  // can be seen issue: #5497.
  @Override
  public String getTagName() {
    return mChild != null ? mChild.getTagName() : null;
  }

  // See the above note for the reason why override this function when enable new flatten.
  @Override
  public String getIdSelector() {
    return mChild != null ? mChild.getIdSelector() : null;
  }

  // See the above note.
  @Override
  public String getRefIdSelector() {
    return mChild != null ? mChild.getRefIdSelector() : null;
  }

  @Override
  public void insertDrawList(LynxBaseUI mark, LynxBaseUI child) {
    ((LynxUI) mChild).insertDrawList(mark, child);
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    if (mChild == null) {
      return null;
    }
    return mChild.hitTest(x, y, ignoreUserInteraction);
  }

  @Override
  public boolean containsPoint(float x, float y) {
    return containsPoint(x, y, false);
  }

  @Override
  public boolean containsPoint(float x, float y, boolean ignoreUserInteraction) {
    if (mChild == null) {
      return false;
    }
    return mChild.containsPoint(x, y, ignoreUserInteraction);
  }

  @Override
  public boolean childrenContainPoint(float x, float y) {
    if (mChild == null) {
      return false;
    }
    return mChild.childrenContainPoint(x, y);
  }

  @Override
  public void insertView(LynxUI child) {
    ((UIGroup) mChild).insertView((child));
  }

  @Override
  public boolean isInsertViewCalled() {
    if (mChild instanceof UIGroup) {
      return ((UIGroup) mChild).isInsertViewCalled();
    }
    return super.isInsertViewCalled();
  }

  @Override
  public boolean isFlatten() {
    return mChild.isFlatten();
  }
  @Override
  public List<LynxBaseUI> getChildren() {
    return mChild.getChildren();
  }

  @Override
  public LynxBaseUI getChildAt(int index) {
    return mChild.mChildren.get(index);
  }

  @Override
  public int getChildCount() {
    if (mChild instanceof UIGroup) {
      return ((UIGroup<?>) mChild).getChildCount();
    }
    return 0;
  }

  @Override
  protected void initAccessibilityDelegate() {
    super.initAccessibilityDelegate();
    // Note: ShadowView should always be important for accessibility because the accessibility node
    // in ShadowView will be set traversal order when initialized.
    ViewCompat.setImportantForAccessibility(mView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
  }

  public LynxBaseUI getChild() {
    return mChild;
  }

  protected ShadowView createView(Context context) {
    return new ShadowView(this, context);
  }

  public void setOutlineStyle(@Nullable BorderStyle style) {
    getOrCreateOutline().mStyle = style;
  }

  public void setOutlineColor(int color) {
    getOrCreateOutline().mColor = color;
  }

  public void setOutlineWidth(float width) {
    getOrCreateOutline().mWidth = width;
  }

  private Outline getOrCreateOutline() {
    if (mOutline == null) {
      mOutline = new Outline();
      if (mView != null) {
        mView.updateOutlineDrawer(mOutline);
      }
    }
    return mOutline;
  }

  @Override
  public void updateLayout(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    mChild.updateLayout(left, top, width, height, paddingLeft, paddingTop, paddingRight,
        paddingBottom, marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth,
        borderTopWidth, borderRightWidth, borderBottomWidth, bound);
    onLayoutUpdated();
  }

  @Override
  public void onLayoutUpdated() {
    setNeedUpdateShadow(true);
  }
  @Override
  public void updateSticky(float[] sticky) {
    mChild.updateSticky(sticky);
    invalidate();
  }

  public void updateTransform() {
    mTransform = null;
    mTransformToUpdate = null != mChild.getTransformRaws();
  }

  private void autoUpdateTransformMatrix() {
    if (mTransform != null || !mTransformToUpdate)
      return;

    mTransformToUpdate = false;

    if (null == mChild.mTransformRaw) {
      mTransform = null;
      return;
    }

    final Rect rect = getChildFrameRect();
    mTransform = TransformProps.processTransform(mChild.mTransformRaw,
        mContext.getUIBody().getFontSize(), getFontSize(), mContext.getUIBody().getWidth(),
        mContext.getUIBody().getHeight(), rect.width(), rect.height());
  }

  @Override
  protected Rect getBoundRectForOverflow() {
    return null;
  }

  @Override
  public Rect getBound() {
    return mChild.getBound();
  }

  @Override
  public int getWidth() {
    return mChild.getWidth();
  }

  @Override
  public int getHeight() {
    return mChild.getHeight();
  }

  @Override
  public int getTop() {
    return mChild.getTop();
  }

  @Override
  public int getLeft() {
    return mChild.getLeft();
  }

  @Override
  public int getOriginTop() {
    return mChild.getOriginTop();
  }

  @Override
  public int getOriginLeft() {
    return mChild.getOriginLeft();
  }

  @Override
  public void setLeft(int left) {
    mChild.setLeft(left);
  }

  @Override
  public void setTop(int top) {
    mChild.setTop(top);
  }

  @Override
  public void setBound(Rect bound) {
    mChild.setBound(bound);
  }

  @Override
  public void layout() {
    View superView = (View) mView.getParent();
    if (superView == null)
      return;
    mView.layout(0, 0, superView.getWidth(), superView.getHeight());
    if (mView.getParent() instanceof ViewGroup) {
      ViewGroup parent = (ViewGroup) mView.getParent();
      parent.setClipChildren(false);
      ViewCompat.setClipBounds(mView, getBoundRectForOverflow());
    }
    if (mChild instanceof LynxUI) {
      ((LynxUI) mChild).layout();
    }
  }

  @Override
  public TransOffset getTransformValue(float left, float right, float top, float bottom) {
    return mChild.getTransformValue(left, right, top, bottom);
  }

  @Override
  public boolean updateDrawingLayoutInfo(int left, int top, Rect bounds) {
    if (mChild.updateDrawingLayoutInfo(left, top, bounds)) {
      setNeedUpdateShadow(true);
      invalidate();
      return true;
    }
    return false;
  }

  @Override
  public void onDrawingPositionChanged() {
    super.onDrawingPositionChanged();
    setNeedUpdateShadow(true);
  }

  @Override
  public void measure() {
    mChild.measure();
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    mChild.insertChild(child, index);
  }

  @Override
  public void removeChild(LynxBaseUI child) {
    mChild.removeChild(child);
  }

  @Override
  public JavaOnlyMap getProps() {
    return mChild.getProps();
  }

  @Override
  public void removeView(LynxBaseUI child) {
    ((UIGroup) mChild).removeView(child);
  }

  @Override
  public void updateExtraData(Object data) {
    mChild.updateExtraData(data);
  }

  @Override
  public void updateLayoutInfo(LynxBaseUI layout) {
    mChild.updateLayoutInfo(layout);
  }
  @Override
  public void setBoxShadow(@Nullable ReadableArray shadow) {
    // Fixme(liyun):workaround for spring. below 6.0 will crash sometimes.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      return;
    }
    mBoxShadowList = parseShadow(shadow);
    updateShadowAndOutline();
  }

  public float getAlpha() {
    return mChild instanceof LynxUI ? ((LynxUI) mChild).getView().getAlpha() : 1.0f;
  }
  public Matrix getMatrix() {
    mMatrix.reset();
    if (!(mChild instanceof LynxUI)) {
      return mMatrix;
    }
    View childView = ((LynxUI) mChild).getView();
    if (childView != null) {
      float pivotX = childView.getPivotX() + getLeft();
      float pivotY = childView.getPivotY() + getTop();

      float translateX = childView.getTranslationX();
      float translateY = childView.getTranslationY();
      mMatrix.preTranslate(translateX, translateY);

      float scaleX = childView.getScaleX();
      float scaleY = childView.getScaleY();
      mMatrix.preScale(scaleX, scaleY, pivotX, pivotY);

      float rotation = childView.getRotation();
      mMatrix.preRotate(rotation, pivotX, pivotY);
    }

    return mMatrix;
  }

  @Override
  public float getTranslationZ() {
    return mChild instanceof LynxUI ? ((LynxUI) mChild).getTranslationZ() : 0;
  }

  @Override
  public float getRealTimeTranslationZ() {
    return mChild instanceof LynxUI ? ((LynxUI) mChild).getRealTimeTranslationZ() : 0;
  }

  public void notifyAnimating() {
    invalidate();
    setNeedUpdateShadow(true);
  }

  @Override
  public void onPropsUpdated() {
    if (mChild != null) {
      mChild.onPropsUpdated();
    }
    super.onPropsUpdated();
    setNeedUpdateShadow(true);
  }

  @Override
  public boolean checkStickyOnParentScroll(int l, int t) {
    boolean ret = mChild.checkStickyOnParentScroll(l, t);
    if (ret) {
      invalidate();
    }
    return ret;
  }

  private Rect getChildFrameRect() {
    return new Rect(mChild.getLeft(), mChild.getTop(), mChild.getLeft() + mChild.getWidth(),
        mChild.getTop() + mChild.getHeight());
  }

  private void updateShadowAndOutline() {
    float[] borderArray = null;
    BorderRadius borderRadiusObj = mChild.getLynxBackground().getBorderRadius();
    int width = mChild.getWidth(), height = mChild.getHeight();
    if (width == 0 || height == 0) {
      mView.clearShadowDrawerList();
      mView.clearOutlineDrawer();
      return;
    }
    if (borderRadiusObj != null) {
      if (width > 0 && height > 0) {
        borderRadiusObj.updateSize(width, height);
      }
      borderArray = borderRadiusObj.getArray();
    }
    if (width > 0 && height > 0) {
      final Rect childRect = getChildFrameRect();
      if (mOutline != null) {
        mOutline.mFrameRect = childRect;
      }
      mView.updateOutlineDrawer(mOutline);
      mView.updateShadow(mBoxShadowList, childRect, borderArray);
    }

    if (mView.hasInset()) {
      InsetDrawer insetDrawer = (mChild != null && mChild.getLynxBackground() != null)
          ? mChild.getLynxBackground().getBoxShadowInsetDrawer()
          : null;
      if (insetDrawer == null) {
        insetDrawer = new InsetDrawer(mView);
        mChild.getLynxBackground().setBoxShadowInsetDrawer(insetDrawer);
      }
      insetDrawer.updateUIPosition(mChild.getLeft(), mChild.getTop());
    }

    mView.invalidate();
  }

  public static class Outline {
    public @Nullable BorderStyle mStyle;
    public Integer mColor = null;
    public float mWidth = 0;
    public Rect mFrameRect;
    private Paint mPaint;
    private Path mPath;

    public Outline() {
      mStyle = BorderStyle.NONE;
      mColor = Color.BLACK;
    }

    protected void onDraw(Canvas canvas) {
      if (mStyle == null || mStyle.equals(BorderStyle.NONE) || mStyle.equals(BorderStyle.HIDDEN)) {
        return;
      }
      if (mWidth < 1e-3) {
        return;
      }
      if (mFrameRect == null || mFrameRect.width() < 1 || mFrameRect.height() < 1) {
        return;
      }

      final int saved = canvas.save();

      doDraw(canvas);

      canvas.restoreToCount(saved);
    }

    private void clipQuadrilateral(Canvas canvas, float x1, float y1, float x2, float y2, float x3,
        float y3, float x4, float y4) {
      if (mPath == null) {
        mPath = new Path();
      }
      mPath.reset();
      mPath.moveTo(x1, y1);
      mPath.lineTo(x2, y2);
      mPath.lineTo(x3, y3);
      mPath.lineTo(x4, y4);
      mPath.lineTo(x1, y1);

      canvas.clipPath(mPath);
    }

    private void doDraw(Canvas canvas) {
      final Rect bound = mFrameRect;
      final int left = bound.left;
      final int top = bound.top;
      final int width = bound.width();
      final int height = bound.height();
      final int lineWidth = (mWidth < 1 ? 1 : Math.round(mWidth));

      mPaint = new Paint();
      mPaint.setAntiAlias(false);
      mPaint.setStyle(Paint.Style.STROKE);

      final int color = mColor != null ? mColor : 0xFF000000;

      {
        final float x1 = left;
        final float y1 = top;
        final float x2 = left - lineWidth;
        final float y2 = top - lineWidth;
        final float x3 = left + width + lineWidth;
        final float y3 = top - lineWidth;
        final float x4 = left + width;
        final float y4 = top;
        final float y5 = top - lineWidth * 0.5f;
        canvas.save();
        clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4);

        mStyle.strokeBorderLine(canvas, mPaint, Spacing.TOP, mWidth, color, x2, y5, x3, y5,
            width + lineWidth * 2, lineWidth);
        canvas.restore();
      }

      {
        final float x1 = left + width;
        final float y1 = top;
        final float x2 = left + width;
        final float y2 = top + height;
        final float x3 = left + width + lineWidth;
        final float y3 = top + height + lineWidth;
        final float x4 = left + width + lineWidth;
        final float y4 = top - lineWidth;

        final float x5 = left + width + lineWidth * 0.5f;
        canvas.save();
        clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4);
        mStyle.strokeBorderLine(canvas, mPaint, Spacing.RIGHT, mWidth, color, x5, y4, x5, y3,
            height + lineWidth * 2, lineWidth);
        canvas.restore();
      }

      {
        final float x1 = left;
        final float y1 = top + height;
        final float x2 = left + width;
        final float y2 = top + height;
        final float x3 = left + width + lineWidth;
        final float y3 = top + height + lineWidth;
        final float x4 = left - lineWidth;
        final float y4 = top + height + lineWidth;

        final float y5 = top + height + lineWidth * 0.5f;
        canvas.save();
        clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4);
        mStyle.strokeBorderLine(canvas, mPaint, Spacing.BOTTOM, mWidth, color, x3, y5, x4, y5,
            width + lineWidth * 2, lineWidth);
        canvas.restore();
      }

      {
        final float x1 = left;
        final float y1 = top;
        final float x2 = left - lineWidth;
        final float y2 = top - lineWidth;
        final float x3 = left - lineWidth;
        final float y3 = top + height + lineWidth;
        final float x4 = left;
        final float y4 = top + height;

        final float x5 = left - lineWidth * 0.5f;
        canvas.save();
        clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4);
        mStyle.strokeBorderLine(canvas, mPaint, Spacing.LEFT, mWidth, color, x5, y3, x5, y2,
            height + lineWidth * 2, lineWidth);
        canvas.restore();
      }
      mPaint.setAntiAlias(true);
    }
  }

  public static class ShadowView extends AndroidView {
    private boolean mHasInset;
    public ShadowView(UIShadowProxy proxy, Context context) {
      super(context);
      mShadowProxy = new WeakReference<UIShadowProxy>(proxy);
      setWillNotDraw(false);
    }

    public boolean hasInset() {
      return mHasInset;
    }
    public void updateOutlineDrawer(Outline outline) {
      mOutlineDrawer = outline;
    }

    public void updateShadow(
        List<ShadowData> shadowList, Rect childBounds, float[] cornerRadiusArray) {
      mShadowDrawerList = null;
      if (shadowList == null || shadowList.isEmpty())
        return;

      mShadowDrawerList = new ArrayList();
      for (ShadowData shadow : shadowList) {
        ShadowDrawer drawer = new ShadowDrawer();
        drawer.updateShadow(shadow, childBounds, cornerRadiusArray);
        if (drawer.mBoxShadow != null && drawer.mBoxShadow.isInset()) {
          mHasInset = true;
        }
        mShadowDrawerList.add(drawer);
      }
      invalidate();
    }

    public void clearShadowDrawerList() {
      if (mShadowDrawerList != null) {
        mShadowDrawerList.clear();
      }
      invalidate();
    }

    public void clearOutlineDrawer() {
      mOutlineDrawer = null;
    }

    public void drawInset(Canvas canvas) {
      if ((mShadowDrawerList == null || mShadowDrawerList.isEmpty()) && mOutlineDrawer == null) {
        return;
      }

      // draw above background & below the real children
      if (mShadowDrawerList != null) {
        for (int i = mShadowDrawerList.size() - 1; i >= 0; i--) {
          ShadowDrawer drawer = mShadowDrawerList.get(i);
          if (drawer != null && drawer.mBoxShadow.isInset()) {
            drawer.onDraw(canvas);
          }
        }
      }
      if (mOutlineDrawer != null) {
        mOutlineDrawer.onDraw(canvas);
      }
    }

    private void drawOutset(final Canvas canvas) {
      if ((mShadowDrawerList == null || mShadowDrawerList.isEmpty()) && mOutlineDrawer == null) {
        return;
      }

      UIShadowProxy proxy = mShadowProxy.get();
      float alpha = proxy != null ? proxy.getAlpha() : 1.0f;
      if (alpha <= 0.001f) {
        return;
      }
      if (alpha < 1.0f) {
        canvas.saveLayerAlpha(
            0, 0, getWidth(), getHeight(), (int) (alpha * 255), Canvas.ALL_SAVE_FLAG);
      } else {
        canvas.save();
      }
      if (proxy != null) {
        Matrix matrix = mShadowProxy.get().getMatrix();
        if (!matrix.isIdentity()) {
          canvas.concat(matrix);
        }
        // translationZ should set to View
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
          setTranslationZ(mShadowProxy.get().getTranslationZ());
        }
      }

      // draw above children
      if (mShadowDrawerList != null) {
        for (int i = mShadowDrawerList.size() - 1; i >= 0; i--) {
          ShadowDrawer drawer = mShadowDrawerList.get(i);
          if (drawer != null && !drawer.mBoxShadow.isInset()) {
            drawer.onDraw(canvas);
          }
        }
      }

      canvas.restore();
    }

    private void drawOutline(final Canvas canvas) {
      if (mOutlineDrawer != null) {
        mOutlineDrawer.onDraw(canvas);
      }
    }

    @Override
    protected void dispatchDraw(final Canvas canvas) {
      UIShadowProxy proxy = mShadowProxy.get();
      if (proxy != null && proxy.isNeedUpdateShadow()) {
        proxy.updateShadowAndOutline();
        proxy.setNeedUpdateShadow(false);
      }
      // draw outset
      drawOutset(canvas);
      super.dispatchDraw(canvas);

      // draw Outline
      drawOutline(canvas);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {}

    private ArrayList<ShadowDrawer> mShadowDrawerList;
    private Outline mOutlineDrawer;
    private WeakReference<UIShadowProxy> mShadowProxy;
  }

  public static class ShadowDrawer {
    Paint mPaint;
    Paint mCornerShadowPaintLT;
    Paint mCornerShadowPaintRT;
    Paint mCornerShadowPaintLB;
    Paint mCornerShadowPaintRB;
    Paint mEdgeShadowPaint;
    final RectF mChildBounds;
    final RectF mChildOrigBounds;
    float[] mCornerRadiusArray;
    float[] mCornerOrigRadiusArray;

    ShadowData mBoxShadow;
    final Path boxPath = new Path();
    final Path origBoxPath = new Path();

    final Path cornerPathLT = new Path();
    final Path cornerPathRB = new Path();
    final Path cornerPathLB = new Path();
    final Path cornerPathRT = new Path();
    final Path edgePathTop = new Path();
    final Path edgePathBottom = new Path();
    final Path edgePathLeft = new Path();
    final Path edgePathRight = new Path();

    boolean mClipShadowPath = false;

    public ShadowDrawer() {
      mPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.DITHER_FLAG);
      mCornerShadowPaintLT = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.DITHER_FLAG);
      mCornerShadowPaintLT.setStyle(Paint.Style.FILL);
      mCornerShadowPaintLT.setDither(true);
      mCornerShadowPaintRT = new Paint(mCornerShadowPaintLT);
      mCornerShadowPaintLB = new Paint(mCornerShadowPaintLT);
      mCornerShadowPaintRB = new Paint(mCornerShadowPaintLT);
      mChildBounds = new RectF();
      mChildOrigBounds = new RectF();
      mEdgeShadowPaint = new Paint(mCornerShadowPaintLT);
      mCornerRadiusArray = new float[8];
      mCornerOrigRadiusArray = new float[8];
    }

    private float calBorderRadiusAdjustForBound(final RectF rect, float[] borderRadius) {
      if (FloatUtils.floatsEqual(rect.width(), 0) || FloatUtils.floatsEqual(rect.height(), 0)) {
        return 0;
      }

      float val = 0.0f, tmp = 1.0f;
      if (borderRadius[0] + borderRadius[2] > rect.width()) {
        tmp = (borderRadius[0] + borderRadius[2]) - rect.width();
        if (tmp > val)
          val = tmp;
      }
      if (borderRadius[4] + borderRadius[6] > rect.width()) {
        tmp = (borderRadius[4] + borderRadius[6]) - rect.width();
        if (tmp > val)
          val = tmp;
      }
      if (borderRadius[1] + borderRadius[7] > rect.height()) {
        tmp = (borderRadius[1] + borderRadius[7]) - rect.height();
        if (tmp > val)
          val = tmp;
      }
      if (borderRadius[3] + borderRadius[5] > rect.height()) {
        tmp = (borderRadius[3] + borderRadius[5]) - rect.height();
        if (tmp > val)
          val = tmp;
      }
      return val;
    }

    private void adjustBorderRadiusForBound(final RectF rect, float[] borderRadius) {
      if (FloatUtils.floatsEqual(rect.width(), 0) || FloatUtils.floatsEqual(rect.height(), 0)) {
        return;
      }

      float val = 1.0f, tmp = 1.0f;
      if (borderRadius[0] + borderRadius[2] > rect.width()) {
        tmp = rect.width() / (borderRadius[0] + borderRadius[2]);
        if (tmp < val)
          val = tmp;
      }
      if (borderRadius[4] + borderRadius[6] > rect.width()) {
        tmp = rect.width() / (borderRadius[4] + borderRadius[6]);
        if (tmp < val)
          val = tmp;
      }
      if (borderRadius[1] + borderRadius[7] > rect.height()) {
        tmp = rect.height() / (borderRadius[1] + borderRadius[7]);
        if (tmp < val)
          val = tmp;
      }
      if (borderRadius[3] + borderRadius[5] > rect.height()) {
        tmp = rect.height() / (borderRadius[3] + borderRadius[5]);
        if (tmp < val)
          val = tmp;
      }

      if (val < 1.0f) {
        for (int i = 0; i < 8; ++i) {
          borderRadius[i] *= val;
        }
      }
    }

    public void updateShadow(ShadowData shadow, Rect childBounds, float[] cornerRadiusArray) {
      mBoxShadow = shadow;
      if (cornerRadiusArray != null && cornerRadiusArray.length == 8) {
        System.arraycopy(cornerRadiusArray, 0, mCornerOrigRadiusArray, 0, 8);
      } else {
        Arrays.fill(mCornerOrigRadiusArray, 0);
      }
      System.arraycopy(mCornerOrigRadiusArray, 0, mCornerRadiusArray, 0, 8);

      mChildBounds.set(childBounds);
      mChildOrigBounds.set(childBounds);
      if (mBoxShadow != null) {
        float inset =
            (mBoxShadow.spreadRadius - mBoxShadow.blurRadius / 2) * (mBoxShadow.isInset() ? 1 : -1);
        if (inset > 0) {
          float insetMax = Math.min(mChildBounds.width(), mChildBounds.height()) / 2;
          if (inset > insetMax) {
            inset = insetMax;
          }
        }
        mChildBounds.inset(inset, inset);
        mChildBounds.offset(mBoxShadow.offsetX, mBoxShadow.offsetY);
        for (int i = 0; i < 8; ++i) {
          mCornerRadiusArray[i] = Math.max(mCornerRadiusArray[i] - inset, 0);
        }
        adjustBorderRadiusForBound(mChildBounds, mCornerRadiusArray);
      }

      if (mBoxShadow != null) {
        final float[] arr = mCornerRadiusArray;
        buildShadowCorner(cornerPathLT, arr[0], arr[1]);
        buildShadowCorner(cornerPathRT, arr[2], arr[3]);
        buildShadowCorner(cornerPathRB, arr[4], arr[5]);
        buildShadowCorner(cornerPathLB, arr[6], arr[7]);

        buildShadowPaint();

        buildShadowPath();
      }
    }

    /**
     * build shadow path with Path.Op.DIFFERENCE
     */
    private void buildShadowPath() {
      Path pathDiffTop = new Path();
      Path pathDiffBottom = new Path();
      Path pathDiffLeft = new Path();
      Path pathDiffRight = new Path();

      boxPath.reset();
      origBoxPath.reset();

      boxPath.addRoundRect(mChildBounds, mCornerRadiusArray, Path.Direction.CW);
      origBoxPath.addRoundRect(mChildOrigBounds, mCornerOrigRadiusArray, Path.Direction.CW);

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        final Path.Op pathOp =
            (mBoxShadow.isInset() ? Path.Op.REVERSE_DIFFERENCE : Path.Op.DIFFERENCE);
        boxPath.op(origBoxPath, pathOp);
      } else {
        // When OS version lower than 8, Path.op may caused native crash
        // backtrace:
        // SkTSect<SkDConic, SkDConic>::extractCoincident
        // SkTSect<SkDConic, SkDConic>::BinarySearch
        // SkIntersections::intersect
        // AddIntersectTs
        // OpDebug
        mClipShadowPath = true;
        boxPath.addRoundRect(mChildOrigBounds, mCornerOrigRadiusArray, Path.Direction.CCW);
      }

      mPaint.setColor(mBoxShadow.color);

      // prepare pathDiff
      pathDiffTop.reset();
      pathDiffTop.set(origBoxPath);
      pathDiffTop.offset(-mChildBounds.left, -mChildBounds.top);

      Matrix tmp = new Matrix();
      pathDiffBottom.reset();
      pathDiffBottom.set(origBoxPath);
      tmp.reset();
      tmp.preRotate(-180f);
      tmp.preTranslate(-mChildBounds.right, -mChildBounds.bottom);
      pathDiffBottom.transform(tmp);

      pathDiffLeft.reset();
      pathDiffLeft.set(origBoxPath);
      tmp.reset();
      tmp.preRotate(-270f);
      tmp.preTranslate(-mChildBounds.left, -mChildBounds.bottom);
      pathDiffLeft.transform(tmp);

      pathDiffRight.reset();
      pathDiffRight.set(origBoxPath);
      tmp.reset();
      tmp.preRotate(-90f);
      tmp.preTranslate(-mChildBounds.right, -mChildBounds.top);
      pathDiffRight.transform(tmp);

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
        final Path.Op pathOp = (mBoxShadow.isInset() ? Path.Op.INTERSECT : Path.Op.DIFFERENCE);
        if (!cornerPathLT.isEmpty()) {
          cornerPathLT.op(pathDiffTop, pathOp);
        }
        if (!cornerPathRB.isEmpty()) {
          cornerPathRB.op(pathDiffBottom, pathOp);
        }
        if (!cornerPathLB.isEmpty()) {
          cornerPathLB.op(pathDiffLeft, pathOp);
        }
        if (!cornerPathRT.isEmpty()) {
          cornerPathRT.op(pathDiffRight, pathOp);
        }
      }

      // edgePath
      edgePathTop.reset();
      edgePathBottom.reset();
      edgePathLeft.reset();
      edgePathRight.reset();

      final float[] edgArr = mCornerRadiusArray;
      final float startY = (mBoxShadow.isInset() ? 0 : -mBoxShadow.blurRenderRadiusExtent);
      final float endY = (mBoxShadow.isInset() ? mBoxShadow.blurRenderRadiusExtent : 0);

      edgePathTop.addRect(
          edgArr[0], startY, mChildBounds.width() - edgArr[2], endY, Path.Direction.CW);

      edgePathBottom.addRect(
          edgArr[4], startY, mChildBounds.width() - edgArr[6], endY, Path.Direction.CW);

      edgePathLeft.addRect(
          edgArr[7], startY, mChildBounds.height() - edgArr[1], endY, Path.Direction.CW);

      edgePathRight.addRect(
          edgArr[3], startY, mChildBounds.height() - edgArr[5], endY, Path.Direction.CW);

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
        final Path.Op pathOp = (mBoxShadow.isInset() ? Path.Op.INTERSECT : Path.Op.DIFFERENCE);
        if (!edgePathTop.isEmpty()) {
          edgePathTop.op(pathDiffTop, pathOp);
        }
        if (!edgePathBottom.isEmpty()) {
          edgePathBottom.op(pathDiffBottom, pathOp);
        }
        if (!edgePathLeft.isEmpty()) {
          edgePathLeft.op(pathDiffLeft, pathOp);
        }
        if (!edgePathRight.isEmpty()) {
          edgePathRight.op(pathDiffRight, pathOp);
        }
      }
    }

    private void buildShadowCorner(Path path, float radiusX, float radiusY) {
      path.reset();

      float inset = mBoxShadow.isInset() ? mBoxShadow.blurRenderRadiusExtent
                                         : -mBoxShadow.blurRenderRadiusExtent;
      RectF innerBounds = new RectF(0, 0, 2 * radiusX, 2 * radiusY);
      RectF outerBounds = new RectF(innerBounds);
      if (inset > 0) {
        float insetMax = Math.min(outerBounds.width(), outerBounds.height()) / 2;
        if (inset > insetMax) {
          inset = insetMax;
        }
      }
      if ((inset > -0.1f && inset < 0.1f)) {
        // it means the path is empty, ignore it
        return;
      }
      outerBounds.inset(inset, inset);

      path.setFillType(Path.FillType.EVEN_ODD);
      path.moveTo(0, radiusY);
      path.rLineTo(inset, 0);
      // outer arc
      path.arcTo(outerBounds, 180f, 90f, false);
      // inner arc
      path.arcTo(innerBounds, 270f, -90f, false);
      path.close();
    }

    private void buildShadowPaint() {
      float mShadowSize = mBoxShadow.blurRenderRadiusExtent;

      int argb[] = new int[4];
      argb[0] = Color.alpha(mBoxShadow.color);
      argb[1] = Color.red(mBoxShadow.color);
      argb[2] = Color.green(mBoxShadow.color);
      argb[3] = Color.blue(mBoxShadow.color);

      mPaint.setColor(mBoxShadow.color);

      int[] colors = new int[multiplier.length];
      for (int i = 0; i < multiplier.length; i++) {
        int aa = (int) (argb[0] * multiplier[i]);
        colors[i] = Color.argb(aa, argb[1], argb[2], argb[3]);
      }

      mEdgeShadowPaint.setShader(new LinearGradient(0, 0, 0,
          mBoxShadow.isInset() ? mShadowSize : -mShadowSize, colors, null, Shader.TileMode.CLAMP));

      final float[] arr = mCornerRadiusArray;
      buildCornerShadowPaint(mCornerShadowPaintLT, argb, arr[0], arr[1]);
      buildCornerShadowPaint(mCornerShadowPaintRT, argb, arr[2], arr[3]);
      buildCornerShadowPaint(mCornerShadowPaintRB, argb, arr[4], arr[5]);
      buildCornerShadowPaint(mCornerShadowPaintLB, argb, arr[6], arr[7]);
    }

    private void buildCornerShadowPaint(
        Paint shadowPaint, int[] argb, float cornerRadiusX, float cornerRadiusY) {
      final float mShadowSize = mBoxShadow.blurRenderRadiusExtent;
      final float radius = (cornerRadiusX + cornerRadiusY) / 2;
      final int multiplierLen = multiplier.length;
      if (mBoxShadow.isInset()) {
        if (radius <= 1e-6) {
          shadowPaint.setShader(null);
          return;
        }
        final float endRatio = mShadowSize / radius;
        float[] localFactor = new float[multiplierLen + 1];
        int[] localColor = new int[multiplierLen + 1];
        localFactor[multiplierLen] = 1;
        localColor[multiplierLen] = mBoxShadow.color;
        for (int i = 1; i < multiplierLen; i++) {
          localFactor[multiplierLen - i] = Math.max(1 - endRatio * i / (multiplierLen - 1), 0);
          localColor[multiplierLen - i] =
              Color.argb((int) (argb[0] * multiplier[i]), argb[1], argb[2], argb[3]);
        }
        localFactor[0] = 0;
        localColor[0] = Color.argb(0, argb[1], argb[2], argb[3]);
        shadowPaint.setShader(new RadialGradient(
            cornerRadiusX, cornerRadiusY, radius, localColor, localFactor, Shader.TileMode.CLAMP));
      } else {
        if (radius + mShadowSize <= 1e-6) {
          shadowPaint.setShader(null);
          return;
        }
        final float startRatio = radius / (radius + mShadowSize);
        float[] localFactor = new float[multiplierLen + 1];
        int[] localColor = new int[multiplierLen + 1];
        localFactor[0] = 0;
        localFactor[1] = startRatio;
        localColor[0] = mBoxShadow.color;
        localColor[1] = mBoxShadow.color;
        for (int i = 2; i <= multiplierLen; i++) {
          localFactor[i] = startRatio + (1 - startRatio) * (i - 1) / (multiplierLen - 1);
          localColor[i] =
              Color.argb((int) (argb[0] * multiplier[i - 1]), argb[1], argb[2], argb[3]);
        }
        shadowPaint.setShader(new RadialGradient(cornerRadiusX, cornerRadiusY, radius + mShadowSize,
            localColor, localFactor, Shader.TileMode.CLAMP));
      }
    }

    protected void onDraw(Canvas canvas) {
      if (mBoxShadow == null) {
        return;
      }
      int saved = canvas.save();

      if (mClipShadowPath) {
        canvas.clipPath(
            origBoxPath, mBoxShadow.isInset() ? Region.Op.INTERSECT : Region.Op.DIFFERENCE);
      }

      canvas.drawPath(boxPath, mPaint);
      try {
        drawShadow(canvas);
      } catch (Exception e) {
        LLog.e(TAG, "Exception occurred while drawing shadow: " + e.getStackTrace().toString());
      }
      canvas.restoreToCount(saved);
    }

    private void drawShadow(Canvas canvas) {
      if (mBoxShadow == null)
        return;

      // Top Shadow
      int saved = canvas.save();
      canvas.translate(mChildBounds.left, mChildBounds.top);
      canvas.drawPath(cornerPathLT, mCornerShadowPaintLT);
      canvas.drawPath(edgePathTop, mEdgeShadowPaint);
      canvas.restoreToCount(saved);

      // Bottom Shadow
      saved = canvas.save();
      canvas.translate(mChildBounds.right, mChildBounds.bottom);
      canvas.rotate(180f);
      canvas.drawPath(cornerPathRB, mCornerShadowPaintRB);
      canvas.drawPath(edgePathBottom, mEdgeShadowPaint);
      canvas.restoreToCount(saved);

      // Left Shadow
      saved = canvas.save();
      canvas.translate(mChildBounds.left, mChildBounds.bottom);
      canvas.rotate(270f);
      canvas.drawPath(cornerPathLB, mCornerShadowPaintLB);
      canvas.drawPath(edgePathLeft, mEdgeShadowPaint);
      canvas.restoreToCount(saved);

      // Right Shadow
      saved = canvas.save();
      canvas.translate(mChildBounds.right, mChildBounds.top);
      canvas.rotate(90f);
      canvas.drawPath(cornerPathRT, mCornerShadowPaintRT);
      canvas.drawPath(edgePathRight, mEdgeShadowPaint);
      canvas.restoreToCount(saved);
    }

    /// The multiplier is calculated by the formula: pow(3, -2*x), x = [0:0.1:0.9], in order to
    /// simulate the gaussian blur's opacity change.
    static double[] multiplier = new double[] {1.0, 0.8027415617602307, 0.6443940149772542,
        0.5172818579717866, 0.41524364653850576, 0.3333333333333333, 0.2075805205867436,
        0.1147980049924181, 0.0424272859905955, 0.00};
  }

  public static class InsetDrawer {
    private ShadowView mShadowView;
    // childView's Postion
    private float mLeft;
    private float mTop;

    InsetDrawer(ShadowView view) {
      mShadowView = view;
    }

    public void updateUIPosition(float left, float top) {
      mLeft = left;
      mTop = top;
    }

    public void draw(Canvas canvas) {
      if (mShadowView != null) {
        int saved = canvas.save();
        canvas.translate(-mLeft, -mTop);
        mShadowView.drawInset(canvas);
        canvas.restoreToCount(saved);
      }
    }
  }

  @Override
  public boolean enableLayoutAnimation() {
    return mChild.enableLayoutAnimation();
  }

  @Nullable
  @Override
  public LayoutAnimationManager getLayoutAnimator() {
    return mChild.getLayoutAnimator();
  }

  @Override
  public TransitionAnimationManager getTransitionAnimator() {
    return mChild.getTransitionAnimator();
  }

  @Override
  public void updatePropertiesInterval(StylesDiffMap props) {
    getChild().updateProperties(props);
    onPropsUpdated();
  }

  @Override
  public boolean isFirstAnimatedReady() {
    return mChild.isFirstAnimatedReady();
  }
}
