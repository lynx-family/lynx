/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static androidx.core.view.ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_AUTO;
import static androidx.core.view.ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS;
import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_RTL;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_BLUR;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_GRAYSCALE;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_NONE;
import static com.lynx.tasm.behavior.StyleConstants.PLATFORM_PERSPECTIVE_UNIT_DEFAULT;
import static com.lynx.tasm.behavior.StyleConstants.PLATFORM_PERSPECTIVE_UNIT_NUMBER;
import static com.lynx.tasm.behavior.StyleConstants.PLATFORM_PERSPECTIVE_UNIT_VH;
import static com.lynx.tasm.behavior.StyleConstants.PLATFORM_PERSPECTIVE_UNIT_VW;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.VISIBILITY_VISIBLE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_DEFAULT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PointF;
import android.os.Build;
import android.text.TextUtils;
import android.util.Base64;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.animation.AnimationConstant;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.animation.keyframe.KeyframeManager;
import com.lynx.tasm.animation.keyframe.LynxKeyframeAnimator;
import com.lynx.tasm.animation.layout.LayoutAnimationManager;
import com.lynx.tasm.animation.transition.TransitionAnimationManager;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.herotransition.HeroAnimOwner;
import com.lynx.tasm.behavior.herotransition.HeroTransitionManager;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.shapes.BasicShape;
import com.lynx.tasm.behavior.ui.text.AndroidText;
import com.lynx.tasm.behavior.ui.utils.BackgroundManager;
import com.lynx.tasm.behavior.ui.utils.TransformRaw;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.featurecount.LynxFeatureCounter;
import com.lynx.tasm.service.ILynxSystemInvokeService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.BitmapUtils;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.FloatUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.util.List;

/**
 * Responsible for rendering the view
 */
public abstract class LynxUI<T extends View> extends LynxBaseUI {
  private static final String TAG = "LynxUI";
  protected T mView;

  private float mGrayscaleAmount = 1.0f;

  private BackgroundManager mBackgroundManager;
  private boolean mSetVisibleByCSS = true;
  private HeroAnimOwner mHeroAnimOwner;
  @Nullable private KeyframeManager mKeyframeManager;
  @Nullable private LayoutAnimationManager mLayoutAnimator = null;
  @Nullable private TransitionAnimationManager mTransitionAnimator;
  private boolean mOverlappingRendering = true;
  private boolean mEnableReuseAnimationState = true;
  private AnimationInfo[] mAnimationInfos = null;
  protected boolean mFirstRender = true;
  private boolean shouldDoTransformTransition() {
    return !mIsFirstAnimatedReady && hasTransformChanged && null != mTransitionAnimator
        && mTransitionAnimator.containTransition(AnimationConstant.PROP_TRANSFORM);
  }

  // List that contains all UIs should draw during dispatchDraw
  protected LynxBaseUI mDrawHead = null;

  public void setDrawHead(LynxBaseUI ui) {
    mDrawHead = ui;
  }
  public LynxBaseUI getDrawHead() {
    return mDrawHead;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void insertDrawList(LynxBaseUI pre, LynxBaseUI child) {
    child.setDrawParent(this);
    if (pre == null) {
      // index == 0 && parent is not flatten
      // insert to the start of the list and be the new head.
      if (mDrawHead != null) {
        mDrawHead.mPreviousDrawUI = child;
        child.mNextDrawUI = mDrawHead;
      }
      mDrawHead = child;
    } else {
      // Insert child after mark, mark is child's precursor found in function insertIntoDrawList
      LynxBaseUI next = pre.mNextDrawUI;
      if (next != null) {
        next.mPreviousDrawUI = child;
        child.mNextDrawUI = next;
      }
      child.mPreviousDrawUI = pre;
      pre.mNextDrawUI = child;
    }
  }
  protected BasicShape mClipPath;

  @Override
  public void onDrawingPositionChanged() {
    if (!mView.isLayoutRequested()) {
      handleLayout();
      invalidate();
    }
  }

  // Currently, only enable zIndex prior to API 21 for supporting translateZ
  protected static final boolean ENABLE_ZINDEX =
      Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP;

  @Deprecated
  public LynxUI(Context context) {
    this((LynxContext) context);
  }

  public LynxUI(LynxContext context) {
    this(context, null);
  }

  public LynxUI(LynxContext context, Object param) {
    super(context, param);
  }

  @Override
  public void initialize() {
    super.initialize();
    mView = createView(mContext, mParam);
    if (mView == null) {
      mView = createView(mContext);
    }
    if (mView == null) {
      return;
    }
    mHeroAnimOwner = new HeroAnimOwner(this);
    setLynxBackground(mBackgroundManager = new BackgroundManager(this, getLynxContext()));
    mBackgroundManager.setDrawableCallback(mDrawableCallback);
    // Created CustomAccessibilityDelegateCompat and set to mView if needed.
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null && wrapper.enableHelper()) {
      initAccessibilityDelegate();
    }
  }

  @Override
  public void onDetach() {
    super.onDetach();
    if (null != mKeyframeManager) {
      mKeyframeManager.onDetach();
    }
    if (this.mLayoutAnimator != null) {
      this.mLayoutAnimator.applyLatestLayoutInfoToUI();
    }
  }

  @Override
  public void onAttach() {
    super.onAttach();
    if (null != mKeyframeManager) {
      mKeyframeManager.onAttach();
    }
  }

  protected T createView(Context context) {
    return null;
  }

  protected T createView(Context context, Object param) {
    return null;
  }

  public T getView() {
    return mView;
  }

  @Override
  public void setSign(int sign, String tagName) {
    super.setSign(sign, tagName);
  }

  @LynxProp(name = PropsConstants.OPACITY, defaultFloat = 1.0f)
  public void setAlpha(float alpha) {
    if (getKeyframeManager() != null) {
      getKeyframeManager().notifyPropertyUpdated(LynxKeyframeAnimator.sAlphaStr, alpha);
    }
    if (mTransitionAnimator != null
        && mTransitionAnimator.containTransition(AnimationConstant.PROP_OPACITY)) {
      mTransitionAnimator.applyPropertyTransition(this, AnimationConstant.PROP_OPACITY, alpha);
      return;
    }

    if (alpha != mView.getAlpha()) {
      mView.setAlpha(alpha);
    }
    if (mLayoutAnimator != null) {
      mLayoutAnimator.updateAlpha(alpha);
    }
  }

  @Override
  public float getTranslationX() {
    return mView.getTranslationX();
  }

  @Override
  public float getTranslationY() {
    return mView.getTranslationY();
  }

  @Override
  public float getRealTimeTranslationZ() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      return mView.getTranslationZ();
    }
    return mTranslationZ;
  }

  @Override
  protected float getScaleX() {
    if (mView == null) {
      return 1;
    }
    return mView.getScaleX();
  }

  @Override
  protected float getScaleY() {
    if (mView == null) {
      return 1;
    }
    return mView.getScaleY();
  }

  @Override
  public void onFocusChanged(boolean hasFocus, boolean isFocusTransition) {
    if (!isFocusTransition) {
      InputMethodManager manager = LynxEnv.inst().getInputMethodManager();
      if (manager == null) {
        LLog.w(TAG, "Failed to get InputMethodManager");
        return;
      }
      if (hasFocus) {
        manager.showSoftInput(mView, InputMethodManager.SHOW_IMPLICIT);
      } else {
        manager.hideSoftInputFromWindow(mView.getWindowToken(), 0);
      }
    }
  }
  @LynxProp(name = PropsConstants.VISIBILITY, defaultInt = VISIBILITY_VISIBLE)
  public void setVisibility(int visibility) {
    if (mTransitionAnimator != null
        && mTransitionAnimator.containTransition(AnimationConstant.PROP_VISIBILITY)) {
      mTransitionAnimator.applyPropertyTransition(
          this, AnimationConstant.PROP_VISIBILITY, visibility);
      return;
    }
    int viewVisibility = mView.getVisibility();
    if (visibility == VISIBILITY_VISIBLE) {
      mSetVisibleByCSS = true;
      // when style of element is change, it doesn't trigger "onLayoutUpdated", so we need to
      // setVisibility in force
      viewVisibility = View.VISIBLE;
      mView.setVisibility(View.VISIBLE);
    } else if (visibility == StyleConstants.VISIBILITY_HIDDEN) {
      mSetVisibleByCSS = false;
      viewVisibility = View.INVISIBLE;
      mView.setVisibility(View.INVISIBLE);
    }

    if (getParent() instanceof UIShadowProxy) {
      ((UIShadowProxy) getParent()).setVisibilityForView(viewVisibility);
    }
  }

  /**
   *
   * @param visibility View.VISIBLE, View.INVISIBLE
   */
  public void setVisibilityForView(int visibility) {
    if (visibility == View.VISIBLE) {
      mSetVisibleByCSS = true;
      // when style of element is change, it doesn't trigger "onLayoutUpdated", so we need to
      // setVisibility in force
      mView.setVisibility(View.VISIBLE);
    } else if (visibility == View.INVISIBLE) {
      mSetVisibleByCSS = false;
      mView.setVisibility(View.INVISIBLE);
    }
  }

  private void prepareKeyframeManager() {
    if (null == mKeyframeManager) {
      mKeyframeManager = new KeyframeManager(this);
    }
  }
  @Override
  public void setAnimation(@Nullable ReadableArray animationInfos) {
    if (animationInfos == null) {
      if (mKeyframeManager != null) {
        mKeyframeManager.endAllAnimation();
        mKeyframeManager = null;
      }
      mAnimationInfos = null;
      return;
    }
    prepareKeyframeManager();
    int size = animationInfos.size();
    AnimationInfo[] infos = new AnimationInfo[size];
    for (int i = 0; i < size; ++i) {
      infos[i] = AnimationInfo.toAnimationInfo(animationInfos.getArray(i));
    }
    mAnimationInfos = infos;
    mKeyframeManager.setAnimations(infos);
  }

  // use for hero transition
  public void setAnimation(@NonNull AnimationInfo animation) {
    prepareKeyframeManager();
    mKeyframeManager.setAnimation(animation);
  }

  @LynxProp(name = PropsConstants.TRANSFORM)
  public void setTransform(@Nullable ReadableArray transform) {
    mHasTranslateDiff = hasTranslateDiff(transform);
    super.setTransform(transform);
    if (getKeyframeManager() != null) {
      getKeyframeManager().notifyPropertyUpdated(LynxKeyframeAnimator.sTransformStr, mTransformRaw);
    }
  }
  @Override
  public Matrix getTransformMatrix() {
    if (mView == null || mView.getMatrix() == null) {
      return super.getTransformMatrix();
    }
    return mView.getMatrix();
  }

  @LynxProp(name = PropsConstants.TEST_TAG)
  public void setTestID(@Nullable String tag) {
    mView.setTag(tag);
  }

  @LynxProp(name = PropsConstants.RENDER_TO_HARDWARE_TEXTURE)
  public void setRenderToHardwareTexture(boolean useHWTexture) {
    int layerType = View.LAYER_TYPE_NONE;
    if (useHWTexture) {
      layerType = View.LAYER_TYPE_HARDWARE;
      if (null != mContext) {
        LynxFeatureCounter.count(LynxFeatureCounter.JAVA_HARDWARE_LAYER, mContext.getInstanceId());
      }
    }
    mView.setLayerType(layerType, null);
  }

  @LynxProp(name = PropsConstants.SHARED_ELEMENT)
  public void setShareElement(@Nullable final String name) {
    mHeroAnimOwner.setSharedElementName(name);
  }

  @LynxProp(name = PropsConstants.ENTER_TRANSITION_NAME)
  public void setEnterTransitionName(@Nullable ReadableArray name) {
    AnimationInfo info = AnimationInfo.toAnimationInfo(name);
    if (null != info) {
      if (null != mContext) {
        LynxFeatureCounter.count(
            LynxFeatureCounter.JAVA_ENTER_TRANSITION_NAME_ANDROID, mContext.getInstanceId());
      }
      HeroTransitionManager.inst().registerEnterAnim(this, info);
    }
  }

  @LynxProp(name = PropsConstants.EXIT_TRANSITION_NAME)
  public void setExitTransitionName(@Nullable ReadableArray name) {
    AnimationInfo info = AnimationInfo.toAnimationInfo(name);
    if (null != info) {
      if (null != mContext) {
        LynxFeatureCounter.count(
            LynxFeatureCounter.JAVA_EXIT_TRANSITION_NAME_ANDROID, mContext.getInstanceId());
      }
      HeroTransitionManager.inst().registerExitAnim(this, info);
    }
  }

  @LynxProp(name = PropsConstants.PAUSE_TRANSITION_NAME)
  public void setPauseTransitionName(@Nullable ReadableArray name) {
    AnimationInfo info = AnimationInfo.toAnimationInfo(name);
    if (null != info) {
      if (null != mContext) {
        LynxFeatureCounter.count(
            LynxFeatureCounter.JAVA_PAUSE_TRANSITION_NAME_ANDROID, mContext.getInstanceId());
      }
      HeroTransitionManager.inst().registerPauseAnim(this, info);
    }
  }

  @LynxProp(name = PropsConstants.RESUME_TRANSITION_NAME)
  public void setResumeTransitionName(@Nullable ReadableArray name) {
    AnimationInfo info = AnimationInfo.toAnimationInfo(name);
    if (null != info) {
      if (null != mContext) {
        LynxFeatureCounter.count(
            LynxFeatureCounter.JAVA_RESUME_TRANSITION_NAME_ANDROID, mContext.getInstanceId());
      }
      HeroTransitionManager.inst().registerResumeAnim(this, info);
    }
  }

  @LynxProp(name = PropsConstants.OVERLAP)
  public void setOverlap(@Nullable Dynamic overlap) {
    if (overlap == null) {
      mOverlappingRendering = true;
      return;
    }
    ReadableType type = overlap.getType();
    if (type == ReadableType.Boolean) {
      mOverlappingRendering = overlap.asBoolean();
    } else if (type == ReadableType.String) {
      String overlapStr = overlap.asString();
      mOverlappingRendering = overlapStr.equalsIgnoreCase("true");
    }
  }

  @LynxProp(name = PropsConstants.TRANSFORM_ORDER)
  public void setTransformOrder(@Nullable Dynamic transformOrder) {
    if (transformOrder == null) {
      mBackgroundManager.setTransformOrder(true);
      return;
    }
    ReadableType type = transformOrder.getType();
    if (type == ReadableType.Boolean) {
      mBackgroundManager.setTransformOrder(transformOrder.asBoolean());
    } else if (type == ReadableType.String) {
      String transformOrderStr = transformOrder.asString();
      mBackgroundManager.setTransformOrder(transformOrderStr.equalsIgnoreCase("true"));
    }
  }

  @LynxProp(name = PropsConstants.ENABLE_REUSE_ANIMATION_STATE, defaultBoolean = true)
  public void setEnableReuseAnimationState(boolean enableReuseAnimationState) {
    mEnableReuseAnimationState = enableReuseAnimationState;
    if (!enableReuseAnimationState && null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_DISABLE_REUSE_ANIMATION_STATE, mContext.getInstanceId());
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ELEMENTS_HIDDEN, defaultBoolean = false)
  public void setAccessibilityElementsHidden(boolean value) {
    mView.setImportantForAccessibility(
        value ? IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS : IMPORTANT_FOR_ACCESSIBILITY_AUTO);
  }

  @Override
  public void setAccessibilityValue(String value) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      mView.setStateDescription(value);
    }
  }

  @Override
  public void setAccessibilityHeading(boolean value) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      mView.setAccessibilityHeading(value);
    }
  }

  public boolean hasOverlappingRenderingEnabled() {
    if (DeviceUtils.isHuaWei() && !DeviceUtils.is64BitDevice()) {
      // Huawei 32bit machine bug for opacity OOM issue https://t.wtturl.cn/r1XGQqx/, we just
      // force disable overlap rendering
      LLog.i(TAG, "Disable overlap rendering for Huawei 32bit machine");
      return false;
    }
    return mOverlappingRendering;
  }

  @LynxUIMethod
  public void takeScreenshot(ReadableMap params, Callback callback) {
    JavaOnlyMap result = new JavaOnlyMap();
    if (mView == null) {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE, result);
      return;
    }
    final String format = params.getString("format", "jpeg");
    Bitmap.Config config;
    Bitmap.CompressFormat compressFormat;
    String header;
    if (format.equals("png")) {
      config = Bitmap.Config.ARGB_8888;
      compressFormat = Bitmap.CompressFormat.PNG;
      header = "data:image/png;base64,";
    } else {
      config = Bitmap.Config.RGB_565;
      compressFormat = Bitmap.CompressFormat.JPEG;
      header = "data:image/jpeg;base64,";
    }
    final float scale = (float) params.getDouble("scale", 1.0);
    try {
      Bitmap bitmap;
      ILynxSystemInvokeService systemInvokeService =
          LynxServiceCenter.inst().getService(ILynxSystemInvokeService.class);
      if (systemInvokeService != null) {
        bitmap = systemInvokeService.takeScreenshot(mView, config);
      } else {
        bitmap = Bitmap.createBitmap(mView.getWidth(), mView.getHeight(), config);
        final Canvas canvas = new Canvas(bitmap);
        boolean dirty = mView.isDirty();
        mView.draw(canvas);
        if (dirty) {
          mView.postInvalidate();
        }
      }
      if (scale != 1.f) {
        Matrix m = new Matrix();
        m.setScale(scale, scale);
        bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), m, true);
      }
      String data = BitmapUtils.bitmapToBase64(bitmap, compressFormat, 100, Base64.NO_WRAP);
      result.putInt("width", bitmap.getWidth());
      result.putInt("height", bitmap.getHeight());
      result.putString("data", header + data);
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
    } catch (Throwable e) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, result);
    }
  }

  @Override
  public void requestLayout() {
    mView.requestLayout();
  }

  @Override
  @Nullable
  public KeyframeManager getKeyframeManager() {
    return mKeyframeManager;
  }

  @Override
  public void invalidate() {
    mView.invalidate();
  }

  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    boolean visible = getBound() == null || (getBound().width() > 0 && getBound().height() > 0);
    if (visible && mSetVisibleByCSS) {
      mView.setVisibility(View.VISIBLE);
    } else if (!visible) {
      mView.setVisibility(View.GONE);
    }
  }

  @Override
  public void measure() {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = "LynxUI." + getTagName() + "measure";
      TraceEvent.beginSection(traceEvent);
    }
    setLayoutParamsInternal();
    int widthSpec = View.MeasureSpec.makeMeasureSpec(getWidth(), View.MeasureSpec.EXACTLY);
    int heightSpec = View.MeasureSpec.makeMeasureSpec(getHeight(), View.MeasureSpec.EXACTLY);
    mView.measure(widthSpec, heightSpec);
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  /**
   * Setting the correct LayoutParams for Android View so that when other parent
   * View want to get the MeasureSize of this View through calling
   * {@link View#measure(int, int)}, the validate MeasureSize can be achieved.
   */
  protected void setLayoutParamsInternal() {
    if (mParent != null && mParent instanceof UIGroup && ((UIGroup) mParent).needCustomLayout()) {
      ViewGroup.LayoutParams curParams = mView.getLayoutParams();
      ViewGroup.LayoutParams generatePrams = ((UIGroup) mParent).generateLayoutParams(curParams);
      if (generatePrams != null && curParams != generatePrams) {
        updateLayoutParams(generatePrams);
      }
    }
  }

  /**
   * Update the validate size to LayoutParams of Android View.
   *
   * @param params params to be set
   */
  public void updateLayoutParams(ViewGroup.LayoutParams params) {
    if (params == null) {
      throw new RuntimeException("LayoutPrams should not be null");
    }
    params.width = getWidth();
    params.height = getHeight();
    if (params != mView.getLayoutParams()) {
      mView.setLayoutParams(params);
    }
  }

  public void handleLayout() {
    String layoutTrace = null;
    String viewLayoutTrace = null;
    if (TraceEvent.enableTrace()) {
      layoutTrace = "LynxUI." + getTagName() + ".layout";
      viewLayoutTrace = layoutTrace + ".mView";
      TraceEvent.beginSection(layoutTrace);
      TraceEvent.beginSection(viewLayoutTrace);
    }
    mView.layout(getLeft(), getTop(), getLeft() + getWidth(), getTop() + getHeight());
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(viewLayoutTrace);
    }

    if (getParent() instanceof UIShadowProxy) {
      ((UIShadowProxy) getParent()).updateTransform();
    }

    if (mView.getParent() instanceof ViewGroup) {
      ViewGroup parent = (ViewGroup) mView.getParent();
      boolean hasShear = getSkewX() != 0 || getSkewY() != 0;

      // If the view has shearing transfromation, should not clip to size.
      if (getOverflow() != 0x00 || hasShear) {
        parent.setClipChildren(false);
      }

      // If got overflow:hidden and have transform: skew, should not clip children with
      // #view.setClipBounds(Rect bounds); Bounds will be clipped before do #draw(Canvas canvas).
      if (!hasShear) {
        ViewCompat.setClipBounds(mView, getBoundRectForOverflow());
      }

      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
        /**if API prior to 18, when a view is animating outside it's parent's bounds,
         * if parent's parent is setClipChildren(false), should set parent's clipChildren=false,
         * otherwise, the animating view's content may not refresh outside parent's bounds
         **/
        if (hasAnimationRunning() && (getParent() instanceof LynxUI)
            && ((LynxUI) getParent()).getOverflow() != 0x00) {
          parent.setClipChildren(false);
        }
      }
    }
    if ((getOverflow() != 0 && (getWidth() == 0 || getHeight() == 0))
        && mView instanceof AndroidText) {
      // [workaround]textLayout cannot be drawn, if view's width or height is zero
      ((AndroidText) mView).setOverflow(getOverflow());
    }
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(layoutTrace);
    }
  }

  @Override
  public void layout() {
    handleLayout();
  }

  @Override
  public void onAnimationNodeReady() {
    super.onAnimationNodeReady();
    // 1. Apply perspective
    updatePerspectiveToView();

    // 2. Apply transform
    if (shouldDoTransform()) {
      // 2.1 apply transform-origin
      mBackgroundManager.setTransformOrigin(mTransformOrigin);

      // 2.2 apply transform
      if (shouldDoTransformTransition()) {
        mTransitionAnimator.applyTransformTransition(this);
      } else {
        if (mTransitionAnimator != null) {
          mTransitionAnimator.endTransitionAnimator(AnimationConstant.PROP_TRANSFORM);
        }
        mBackgroundManager.setTransform(mTransformRaw);
      }
    }

    // 3. Apply transition
    if (null != mTransitionAnimator) {
      mTransitionAnimator.startTransitions();
    }

    // 4. Apply keyframe
    if (null != mKeyframeManager) {
      mKeyframeManager.notifyAnimationUpdated();
    }

    if (mContext != null && mContext.isTouchMoving() && mHasTranslateDiff) {
      // The front-end modify the animation properties, causing the UI to slide.
      mContext.onPropsChanged(this);
    }
  }

  public int getBackgroundColor() {
    return mBackgroundManager.getBackgroundColor();
  }

  public BackgroundManager getBackgroundManager() {
    return mBackgroundManager;
  }

  @Nullable
  public ReadableMap getKeyframes(String name) {
    if (null != mContext) {
      return mContext.getKeyframes(name);
    }
    return null;
  }

  @Override
  public void initTransitionAnimator(ReadableMap map) {
    if (mTransitionAnimator == null) {
      mTransitionAnimator = new TransitionAnimationManager(getTransitionUI());
    }

    if (!mTransitionAnimator.initializeFromConfig(map)) {
      mTransitionAnimator = null;
    }
  }

  @Override
  public TransitionAnimationManager getTransitionAnimator() {
    return mTransitionAnimator;
  }

  public LynxUI getTransitionUI() {
    return null;
  }

  @Override
  public boolean getVisibility() {
    return mSetVisibleByCSS;
  }

  @Override
  public boolean isVisible() {
    if (mView == null) {
      return false;
    }

    // if view is not visible, return false
    if (mView.getVisibility() != View.VISIBLE) {
      return false;
    }

    // if view's alpha == 0, return false
    if (mView.getAlpha() == 0) {
      return false;
    }

    // if view is not attached to window, return false
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      return mView.isAttachedToWindow();
    }
    return mView.getWindowToken() != null;
  }

  @Override
  @LynxProp(name = PropsConstants.ACCESSIBILITY_LABEL)
  public void setAccessibilityLabel(@Nullable Dynamic value) {
    super.setAccessibilityLabel(value);
    if (mView != null) {
      mView.setFocusable(true);
      mView.setContentDescription(getAccessibilityLabel());
    }
  }

  @Override
  public void setAccessibilityElement(@Nullable Dynamic value) {
    super.setAccessibilityElement(value);
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    // Note: If the status is not the default, we need to set the accessibility important. If the
    // status is default, we will use the value of config enableAccessibilityElement.
    if (mView != null && mAccessibilityElementStatus != ACCESSIBILITY_ELEMENT_DEFAULT
        && wrapper != null && wrapper.enableHelper()) {
      ViewCompat.setImportantForAccessibility(mView,
          mAccessibilityElementStatus == ACCESSIBILITY_ELEMENT_TRUE
              ? ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES
              : ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
    }
  }

  public HeroAnimOwner getFakeSharedElementManager() {
    return mHeroAnimOwner;
  }

  public void execEnterAnim(HeroTransitionManager.LynxViewEnterFinishListener listener) {
    mHeroAnimOwner.executeEnterAnim(listener);
  }

  public void execExitAnim(HeroTransitionManager.LynxViewExitFinishListener listener) {
    mHeroAnimOwner.executeExitAnim(listener);
  }

  public void execPauseAnim() {
    mHeroAnimOwner.executePauseAnim();
  }

  public void execResumeAnim() {
    mHeroAnimOwner.executeResumeAnim();
  }

  public void setEnterAnim(AnimationInfo enterAnim) {
    mHeroAnimOwner.setEnterAnim(enterAnim);
  }

  public void setExitAnim(AnimationInfo exitAnim) {
    mHeroAnimOwner.setExitAnim(exitAnim);
  }

  public void setPauseAnim(AnimationInfo pauseAnim) {
    mHeroAnimOwner.setPauseAnim(pauseAnim);
  }

  public void setResumeAnim(AnimationInfo resumeAnim) {
    mHeroAnimOwner.setResumeAnim(resumeAnim);
  }

  public void onAnimationEnd(String animName) {
    mHeroAnimOwner.onAnimationEnd(animName);
  }

  @Override
  public boolean enableLayoutAnimation() {
    return null != mLayoutAnimator && mLayoutAnimator.isValid();
  }

  @Override
  @Nullable
  public LayoutAnimationManager getLayoutAnimator() {
    return mLayoutAnimator;
  }

  private void prepareLayoutAnimator() {
    if (null == mLayoutAnimator) {
      mLayoutAnimator = new LayoutAnimationManager();
    }
  }

  // layout create

  @LynxProp(name = AnimationConstant.C_DURATION)
  public void setLayoutAnimationCreateDuration(@Nullable double duration) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_CREATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.setLayoutAnimationCreateDuration(duration);
  }

  @LynxProp(name = AnimationConstant.C_PROPERTY)
  public void setLayoutAnimationCreateProperty(@Nullable int property) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_CREATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutCreateAnimation().setAnimatedProperty(property);
  }

  public void setLayoutAnimationCreateTimingFunc(@Nullable String timingFunc) {}

  @LynxProp(name = AnimationConstant.C_TIMING_FUNCTION)
  public void setLayoutAnimationCreateTimingFunc(@Nullable ReadableArray timingFunc) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_CREATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutCreateAnimation().setInterpolator(timingFunc);
  }
  @LynxProp(name = AnimationConstant.C_DELAY)
  public void setLayoutAnimationCreateDelay(@Nullable double delay) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_CREATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutCreateAnimation().setDelay((long) delay);
  }
  @LynxProp(name = AnimationConstant.U_DURATION)
  public void setLayoutAnimationUpdateDuration(@Nullable double duration) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_UPDATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.setLayoutAnimationUpdateDuration(duration);
  }

  @LynxProp(name = AnimationConstant.U_PROPERTY)
  public void setLayoutAnimationUpdateProperty(@Nullable int property) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_UPDATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutUpdateAnimation().setAnimatedProperty(property);
  }

  @LynxProp(name = AnimationConstant.U_TIMING_FUNCTION)
  public void setLayoutAnimationUpdateTimingFunc(@Nullable ReadableArray timingFunc) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_UPDATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutUpdateAnimation().setInterpolator(timingFunc);
  }

  @LynxProp(name = AnimationConstant.U_DELAY)
  public void setLayoutAnimationUpdateDelay(@Nullable double delay) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_UPDATE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutUpdateAnimation().setDelay((long) delay);
  }

  // layout delete

  @LynxProp(name = AnimationConstant.D_DURATION)
  public void setLayoutAnimationDeleteDuration(@Nullable double duration) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_DELETE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.setLayoutAnimationDeleteDuration(duration);
  }

  @LynxProp(name = AnimationConstant.D_PROPERTY)
  public void setLayoutAnimationDeleteProperty(@Nullable int property) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_DELETE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutDeleteAnimation().setAnimatedProperty(property);
  }

  @LynxProp(name = AnimationConstant.D_TIMING_FUNCTION)
  public void setLayoutAnimationDeleteTimingFunc(@Nullable ReadableArray timingFunc) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_DELETE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutDeleteAnimation().setInterpolator(timingFunc);
  }

  @LynxProp(name = AnimationConstant.D_DELAY)
  public void setLayoutAnimationDeleteDelay(@Nullable double delay) {
    prepareLayoutAnimator();
    if (null != mContext) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_LAYOUT_ANIMATION_DELETE_ANDROID, mContext.getInstanceId());
    }
    mLayoutAnimator.getLayoutDeleteAnimation().setDelay((long) delay);
  }

  @Override
  public boolean checkStickyOnParentScroll(int l, int t) {
    boolean ret = super.checkStickyOnParentScroll(l, t);
    PointF trans = null;
    if (mSticky != null) {
      trans = new PointF(mSticky.x, mSticky.y);
    }
    mBackgroundManager.setPostTranlate(trans);

    return ret;
  }

  @LynxProp(name = PropsConstants.FILTER)
  public void setFilter(@Nullable ReadableArray filter) {
    if (mView == null) {
      return;
    }
    int type = FILTER_TYPE_NONE;
    double amount = .0f;
    // Array will be null when reset, otherwise a [type, amount, amount, unit] array.
    if (filter != null && filter.size() == 3) {
      type = filter.getInt(0);
      amount = filter.getDouble(1);
    }

    switch (type) {
      case FILTER_TYPE_NONE:
        mView.setLayerType(View.LAYER_TYPE_NONE, null);
        if (mView instanceof AndroidView) {
          ((AndroidView) mView).removeBlur();
        }
        mGrayscaleAmount = 1.0f;
        break;
      case FILTER_TYPE_GRAYSCALE:
        amount = 1 - amount;
        amount = UnitUtils.clamp(amount, 0.0, 1.0);

        if (!FloatUtils.floatsEqual(mGrayscaleAmount, (float) amount)) {
          ColorMatrix colorMatrix = new ColorMatrix();
          colorMatrix.setSaturation((float) amount);
          Paint filterPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
          // TODO(renzhongyue): use RenderEffect for API level 31 and above after upgrading
          // compileSdkVersion to 31.
          filterPaint.setColorFilter(new ColorMatrixColorFilter(colorMatrix));
          mView.setLayerType(View.LAYER_TYPE_HARDWARE, filterPaint);
          mGrayscaleAmount = (float) amount;
        }
        break;
      case FILTER_TYPE_BLUR:
        // Only support AndroidView, implementation for api level under 31 depends on a customized
        // ViewTreeObserver#OnPreDrawListener.
        if (mView instanceof AndroidView) {
          ((AndroidView) mView).setBlur((float) amount);
        }
        break;
      default:
        break;
    }
  }

  public boolean hasAnimationRunning() {
    return (mKeyframeManager != null && mKeyframeManager.hasAnimationRunning())
        || (mTransitionAnimator != null && mTransitionAnimator.hasAnimationRunning())
        || (mView != null && mView.getAnimation() != null);
  }

  public boolean hasTranslateDiff(@Nullable ReadableArray transform) {
    List<TransformRaw> oldTransform = TransformRaw.toTransformRaw(transform);
    boolean translateXDiff =
        Float.compare(TransformRaw.hasXValue(mTransformRaw), TransformRaw.hasXValue(oldTransform))
        != 0;
    boolean translateYDiff =
        Float.compare(TransformRaw.hasYValue(mTransformRaw), TransformRaw.hasYValue(oldTransform))
        != 0;
    return translateXDiff || translateYDiff;
  }

  public boolean isRtl() {
    return mLynxDirection == DIRECTION_RTL;
  }

  @Override
  public boolean isUserInteractionEnabled() {
    // If set user-interaction-enabled="{{false}}" or visibility: hidden, this ui will not be on the
    // response chain.
    return this.userInteractionEnabled && (mView != null && mView.getVisibility() == View.VISIBLE);
  }

  public void updatePerspectiveToView() {
    float perspective = 0;
    float scale = mContext.getScreenMetrics().density;
    if (mPerspective != null && mPerspective.size() > 1
        && mPerspective.getInt(1) != PLATFORM_PERSPECTIVE_UNIT_DEFAULT) {
      if (mPerspective.getInt(1) == PLATFORM_PERSPECTIVE_UNIT_NUMBER) {
        perspective = (float) (mPerspective.getDouble(0) * scale * scale
            * CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER);
      } else {
        if (mPerspective.getInt(1) == PLATFORM_PERSPECTIVE_UNIT_VW) {
          perspective =
              (float) (mPerspective.getDouble(0) / 100.0f * mContext.getLynxView().getWidth());
        } else if (mPerspective.getInt(1) == PLATFORM_PERSPECTIVE_UNIT_VH) {
          perspective =
              (float) (mPerspective.getDouble(0) / 100.0f * mContext.getLynxView().getHeight());
        } else {
          perspective = (float) mPerspective.getDouble(0);
        }
        // Make the camera perspective looks the same on Android and iOS.
        // The native Android implementation removed the screen density from the
        // calculation, so squaring and a normalization value of
        // sqrt(5) produces an exact replica with iOS.
        perspective = perspective * scale * CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER;
      }
    } else {
      int maxLength = getWidth() > getHeight() ? getWidth() : getHeight();
      perspective =
          maxLength * scale * CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER * DEFAULT_PERSPECTIVE_FACTOR;
    }
    mView.setCameraDistance(perspective);
  }

  @LynxProp(name = "clip-path")
  public void setClipPath(@Nullable ReadableArray basicShape) {
    mClipPath =
        BasicShape.CreateFromReadableArray(basicShape, mContext.getScreenMetrics().scaledDensity);
  }

  protected void initAccessibilityDelegate() {}

  @Override
  public void destroy() {
    super.destroy();
    if (mTransitionAnimator != null) {
      mTransitionAnimator.onDestroy();
    }
    if (mKeyframeManager != null) {
      mKeyframeManager.onDestroy();
    }
  }

  private void saveKeyframeStateToStorage(String itemKey, LynxBaseUI list, boolean isExist) {
    if (!mEnableReuseAnimationState || TextUtils.isEmpty(itemKey)) {
      return;
    }
    if ((isExist && mKeyframeManager != null) || !isExist) {
      String uniqueAnimationCacheKey = "Animation"
          + "_" + constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
      if (!isExist) {
        list.removeKeyFromNativeStorage(uniqueAnimationCacheKey);
      } else {
        list.storeKeyToNativeStorage(uniqueAnimationCacheKey, mKeyframeManager);
      }
      if (mKeyframeManager != null) {
        mKeyframeManager.detachFromUI();
      }
      mKeyframeManager = null;
    }
  }

  private void restoreKeyframeStateFromStorage(String itemKey, UIList list) {
    if (!mEnableReuseAnimationState || TextUtils.isEmpty(itemKey)) {
      return;
    }
    // When onListCellAppear is triggered, mAnimationInfos has already been updated and is the
    // latest.
    if (mAnimationInfos != null) {
      String uniqueAnimationCacheKey = "Animation"
          + "_" + constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
      KeyframeManager animationManager =
          (KeyframeManager) list.nativeListStateCache.get(uniqueAnimationCacheKey);
      if (animationManager != null) {
        list.nativeListStateCache.remove(uniqueAnimationCacheKey);
        // Before onListCellAppear is triggered, onNodeReady may have already been triggered, which
        // would initiate a complete animation execution from the beginning. We need to clear this
        // animation first.
        if (mKeyframeManager != null) {
          mKeyframeManager.detachFromUI();
          mKeyframeManager = null;
        }
        animationManager.attachToUI(this);
        mKeyframeManager = animationManager;
      } else {
        prepareKeyframeManager();
      }
      mKeyframeManager.setAnimations(mAnimationInfos);
      mKeyframeManager.notifyAnimationUpdated();
    }
  }

  public void onListCellDisAppear(String itemKey, LynxBaseUI list, boolean isExist) {
    super.onListCellDisAppear(itemKey, list, isExist);
    saveKeyframeStateToStorage(itemKey, list, isExist);
  }

  public void onListCellAppear(String itemKey, UIList list) {
    super.onListCellAppear(itemKey, list);
    restoreKeyframeStateFromStorage(itemKey, list);
  }
}
