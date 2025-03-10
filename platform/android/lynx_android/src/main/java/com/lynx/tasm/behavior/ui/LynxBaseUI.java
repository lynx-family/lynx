// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static com.lynx.tasm.behavior.CSSPropertySetter.updateUIPaintStyle;
import static com.lynx.tasm.behavior.StyleConstants.OVERFLOW_VISIBLE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_DEFAULT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_FALSE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.react.bridge.ReadableType;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.animation.AnimationConstant;
import com.lynx.tasm.animation.keyframe.KeyframeManager;
import com.lynx.tasm.animation.keyframe.LynxKeyframeAnimator;
import com.lynx.tasm.animation.layout.LayoutAnimationManager;
import com.lynx.tasm.animation.transition.TransitionAnimationManager;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.*;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityHelper;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.scroll.AbsLynxUIScroll;
import com.lynx.tasm.behavior.ui.scroll.IScrollSticky;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.behavior.ui.utils.LynxMask;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import com.lynx.tasm.behavior.ui.utils.TransformOrigin;
import com.lynx.tasm.behavior.ui.utils.TransformRaw;
import com.lynx.tasm.behavior.utils.PropsUpdater;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.utils.ContextUtils;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.SizeValue;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import org.json.JSONObject;

@LynxUIMethodsHolder
@LynxPropsHolder
public abstract class LynxBaseUI
    implements UIParent, EventTarget, PropertiesDispatcher, Cloneable, LynxNewGestureDelegate {
  public final static int[] SPACING_TYPES = {
      Spacing.ALL,
      Spacing.LEFT,
      Spacing.RIGHT,
      Spacing.TOP,
      Spacing.BOTTOM,
      Spacing.START,
      Spacing.END,
  };

  private final static int[] sDefaultOffsetToLynxView = {Integer.MIN_VALUE, Integer.MIN_VALUE};

  protected static final int DEFAULT_PERSPECTIVE_FACTOR = 100;
  protected static final float CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER = (float) Math.sqrt(5);

  private static final String TAG = "LynxBaseUI";

  // used for drawList
  protected LynxBaseUI mPreviousDrawUI = null;
  protected LynxBaseUI mNextDrawUI = null;
  protected float mTranslationZ = 0;

  // the key of gesture arena member, default value is zero, When it is positive, it means that it
  // has joined the arena
  protected int mGestureArenaMemberId = 0;

  public LynxBaseUI getPreviousDrawUI() {
    return mPreviousDrawUI;
  }

  public void setPreviousDrawUI(LynxBaseUI previousDrawUI) {
    mPreviousDrawUI = previousDrawUI;
  }

  public LynxBaseUI getNextDrawUI() {
    return mNextDrawUI;
  }

  public void setNextDrawUI(LynxBaseUI nextDrawUI) {
    mNextDrawUI = nextDrawUI;
  }

  public class Sticky extends RectF {
    float x, y;
  }

  public class TransOffset {
    public float[] left_top;
    public float[] right_top;
    public float[] right_bottom;
    public float[] left_bottom;
  }

  public static final short OVERFLOW_X = 0x01, OVERFLOW_Y = 0x02, OVERFLOW_XY = 0x03,
                            OVERFLOW_HIDDEN = 0x00;

  protected LynxContext mContext;
  protected Object mParam;
  protected UIParent mParent;

  // UI whose drawList contains this.
  protected UIParent mDrawParent;

  protected boolean mIsTransformNode = false;

  protected final List<LynxBaseUI> mChildren = new ArrayList<>();

  protected List<LynxBaseUI> mStableChildrenForTranslateZ;

  public void resetTranslateZChildrenOrder() {
    mChildren.clear();
    mChildren.addAll(mStableChildrenForTranslateZ);
  }

  public void setStableChildrenForTranslateZ(List<LynxBaseUI> stableChildren) {
    mStableChildrenForTranslateZ = stableChildren;
  }

  public List<LynxBaseUI> GetStableChildrenForTranslateZ() {
    return mStableChildrenForTranslateZ;
  }

  protected LynxBackground mLynxBackground;

  @Nullable protected LynxMask mLynxMask;
  private final JavaOnlyMap mProps = new JavaOnlyMap();

  protected Map<String, EventsListener> mEvents;
  protected Map<Integer, GestureDetector> mGestureDetectors;
  private String mName;
  private String mIdSelector; // id selector used in ttml
  private String mRefId; // ref id selector used in reactLynx
  private ReadableMap mDataset = new JavaOnlyMap();
  private int mSign;
  private int mNodeIndex = 0;
  private int mPseudoStatus;
  private String mTagName;
  private String mTestTagName;
  private String mScrollMonitorTag;
  private boolean mEnableScrollMonitor;
  private Rect mBound;

  // Relative position value to drawParent for drawing.
  private int mLeft;
  private int mTop;
  // Relative position value to parent in UI tree.
  private int mOriginLeft;
  private int mOriginTop;

  private int mWidth;
  private int mHeight;
  protected int mPaddingLeft;
  protected int mPaddingRight;
  protected int mPaddingTop;
  protected int mPaddingBottom;
  protected int mMarginLeft, mMarginTop, mMarginRight, mMarginBottom;
  protected int mBorderTopWidth;
  protected int mBorderLeftWidth;
  protected int mBorderRightWidth;
  protected int mBorderBottomWidth;
  protected float mFontSize;
  private boolean mHasRadius = false;
  // this will be updated by layout change.Not affected by animation.
  // x is width, y is height.
  @NonNull private final Point mLatestSize;
  @NonNull private final Point mLastSize;
  // used to prevent redundant calls to onLayoutUpdated
  private boolean mSkipLayoutUpdated = false;

  @StyleConstants.OverflowType protected int mOverflow = 0;
  // indicates if need to clip children to radius
  private boolean mClipToRadius = false;
  protected boolean mFocusable = false;
  protected EnableStatus mIgnoreFocus = EnableStatus.Undefined;
  protected List<TransformRaw> mTransformRaw;
  @Nullable protected TransformOrigin mTransformOrigin;
  @Nullable protected ReadableArray mPerspective = null;
  protected boolean hasTransformChanged = false;
  protected boolean userInteractionEnabled = true;
  protected boolean nativeInteractionEnabled = false;
  protected Sticky mSticky = null;
  protected float mMaxHeight = -1;
  protected int mBackgroundColor = Color.TRANSPARENT;
  private String mExposureID;
  private String mExposureScene;
  private float mExposureScreenMarginTop;
  private float mExposureScreenMarginBottom;
  private float mExposureScreenMarginLeft;
  private float mExposureScreenMarginRight;
  private float mHitSlopTop;
  private float mHitSlopBottom;
  private float mHitSlopLeft;
  private float mHitSlopRight;
  private Boolean mEnableExposureUIMargin = null;
  private String mExposureUIMarginTop;
  private String mExposureUIMarginBottom;
  private String mExposureUIMarginLeft;
  private String mExposureUIMarginRight;
  private String mExposureArea;

  private boolean mEnableBitmapGradient;

  private int mBorderSpacingIndex;
  private int mBorderWidth;

  public float getSkewX() {
    return mSkewX;
  }

  public void setSkewX(float skewX) {
    this.mSkewX = skewX;
  }

  public float getSkewY() {
    return mSkewY;
  }

  public void setSkewY(float mSkewY) {
    this.mSkewY = mSkewY;
  }

  private float mSkewX = 0;
  private float mSkewY = 0;

  // For fiber: When inertchild is called, do not add the actual view to its ancestor view.
  // Wait until the view tree is fully constructed before inserting.
  private boolean mShouldAttachChildrenView = false;

  private float mExtraOffsetX = 0, mExtraOffsetY = 0;

  // support android-accessibility service
  // default enable accessibility for each lynx node, sometimes front-end will disable it
  private String mAccessibilityLabel = "";
  private String mAccessibilityId = "";
  protected int mAccessibilityElementStatus = ACCESSIBILITY_ELEMENT_DEFAULT;
  protected boolean mAccessibilityEnableTap = false;
  protected boolean mAccessibilityKeepFocused = false;
  private ArrayList<String> mAccessibilityElements;
  private ArrayList<String> mAccessibilityElementsA11y;
  protected boolean mConsumeHoverEvent = false;
  private LynxAccessibilityHelper.LynxAccessibilityTraits mAccessibilityTraits =
      LynxAccessibilityHelper.LynxAccessibilityTraits.NONE;

  private String mAccessibilityRoleDescription;

  private String mAccessibilityStatus;

  private ArrayList mAccessibilityActions;

  protected DrawableCallback mDrawableCallback = new DrawableCallback();
  protected Bitmap.Config mBitmapConfig = null;
  protected int mCSSPosition = StyleConstants.POSITION_RELATIVE;

  // default touch slop is 8dp the same as Android.
  private float mTouchSlop = 8;
  private boolean mOnResponseChain = false;
  // block all native event of this element
  private boolean mBlockNativeEvent = false;
  // block native event at some areas of this element
  private ArrayList<ArrayList<SizeValue>> mBlockNativeEventAreas = null;
  // default event through is undefined
  protected EnableStatus mEventThrough = EnableStatus.Undefined;

  protected int mFlattenChildrenCount = 0;
  private boolean mNeedSortChildren = false;
  private float mLastTranslateZ = 0;
  // default mEnableTouchPseudoPropagation is true
  protected boolean mEnableTouchPseudoPropagation = true;

  private volatile Set<ScrollStateChangeListener> mStateChangeListeners;
  private volatile ScrollStateChangeListener mScrollStateChangeListener;

  private Matrix mTransformMatrix = new Matrix();
  private Matrix mHitTestMatrix = new Matrix();

  private ArrayList<ArrayList<Float>> mConsumeSlideEventAngles = null;
  private boolean mBlockListEvent = false;

  private WeakReference<int[]> mOffsetDescendantRectToLynxView = new WeakReference<int[]>(null);

  private Dynamic mUseLocalCache = null;

  private boolean mSkipRedirection = false;

  protected int mImageRendering = -1;

  // hold boundingClientRect callbacks.
  protected ArrayList<Runnable> mBoundingClientRectCallbacks = new ArrayList<>();

  private CSSPropertySetter.UIPaintStyles mUIPaintStyles;

  // When the onAnimationNodeReady lifecycle is triggered, this flag is used to determine if it is
  // the first time it has been triggered.
  protected boolean mIsFirstAnimatedReady = true;
  protected boolean mHasTranslateDiff = false;

  public boolean isFlatten() {
    return false;
  }

  public boolean canHaveFlattenChild() {
    return true;
  }

  /**
   * lynx direction rtl should use by subview.
   * possible value is:
   * {@link StyleConstants#DIRECTION_LTR} ltr direction;
   * {@link StyleConstants#DIRECTION_RTL} right to left direction;
   */
  protected int mLynxDirection = StyleConstants.DIRECTION_LTR;

  public LynxContext getLynxContext() {
    return mContext;
  }

  public ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams childParams) {
    return null;
  }

  public boolean needCustomLayout() {
    return false;
  }

  public void requestLayout() {}

  public void invalidate() {}

  public void recognizeGesturere() {
    if (mContext != null) {
      mContext.onGestureRecognized(this);
    }
  }

  /**
   * use LynxContext param instead.
   *
   * @param context
   */
  @Deprecated
  protected LynxBaseUI(final Context context) {
    this((LynxContext) context);
  }

  protected LynxBaseUI(final LynxContext context) {
    this(context, null);
  }

  protected LynxBaseUI(final LynxContext context, Object param) {
    mContext = context;
    mParam = param;
    mLynxBackground = new LynxBackground(context);
    mLynxBackground.setDrawableCallback(mDrawableCallback);
    mFontSize = PixelUtils.dipToPx(14, context.getScreenMetrics().density);
    mLynxBackground.setFontSize(mFontSize);
    mLatestSize = new Point();
    mLastSize = new Point();

    initialize();
  }

  public CSSPropertySetter.UIPaintStyles getOrCreateUIPaintStyles() {
    if (mUIPaintStyles == null) {
      mUIPaintStyles = new CSSPropertySetter.UIPaintStyles();
    }
    return mUIPaintStyles;
  }

  public void applyUIPaintStylesToTarget(LynxBaseUI targetUI) {
    updateUIPaintStyle(targetUI, mUIPaintStyles);
  }

  public boolean getVisibility() {
    return true;
  }

  public boolean isVisible() {
    return true;
  }

  public void initialize() {}

  public void destroy() {
    if (this instanceof PatchFinishListener) {
      mContext.unregisterPatchFinishListener((PatchFinishListener) this);
    }
    if (this instanceof ForegroundListener) {
      LynxUIOwner uiOwner = mContext.getLynxUIOwner();
      if (uiOwner != null) {
        uiOwner.unregisterForegroundListener((ForegroundListener) this);
      }
    }
    if (mContext.getIntersectionObserverManager() != null) {
      mContext.getIntersectionObserverManager().removeAttachedIntersectionObserver(this);
    }
    mLynxBackground.onDetach();
    if (mLynxMask != null) {
      mLynxMask.onDetach();
    }
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null) {
      wrapper.addOrRemoveUIFromExclusiveMap(this, false);
    }
  }

  protected void setLynxBackground(LynxBackground lynxBackground) {
    mLynxBackground = lynxBackground;
  }

  public LynxBackground getLynxBackground() {
    return mLynxBackground;
  }

  protected void setLynxMask(LynxMask lynxMask) {
    mLynxMask = lynxMask;
  }

  public LynxMask getLynxMask() {
    return mLynxMask;
  }

  public void setSign(int sign, String tagName) {
    mSign = sign;
    mTagName = tagName;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setNodeIndex(int nodeIndex) {
    mNodeIndex = nodeIndex;
  }

  public int getNodeIndex() {
    return mNodeIndex;
  }

  public String getTagName() {
    return mTagName;
  }

  public void setEvents(Map<String, EventsListener> events) {
    this.mEvents = events;
  }

  // set gesture detectors to lynx ui if set by developer
  public void setGestureDetectors(Map<Integer, GestureDetector> gestureDetectors) {
    this.mGestureDetectors = gestureDetectors;
  }

  public void setParent(UIParent parent) {
    if (mStateChangeListeners == null) {
      mParent = parent;
      return;
    }
    if (parent instanceof LynxBaseUI) {
      // add to parent
      ScrollStateChangeListener[] listeners;
      synchronized (this) {
        listeners = mStateChangeListeners.toArray(
            new ScrollStateChangeListener[mStateChangeListeners.size()]);
      }
      for (ScrollStateChangeListener listener : listeners) {
        ((LynxBaseUI) parent).registerScrollStateListener(listener);
      }
    } else if (mParent instanceof LynxBaseUI) {
      // removed from parent
      ScrollStateChangeListener[] listeners;
      synchronized (this) {
        listeners = mStateChangeListeners.toArray(
            new ScrollStateChangeListener[mStateChangeListeners.size()]);
      }

      for (ScrollStateChangeListener listener : listeners) {
        ((LynxBaseUI) mParent).unRegisterScrollStateListener(listener);
      }
    }
    mParent = parent;
  }

  public void setDrawParent(UIParent drawParent) {
    mDrawParent = drawParent;
  }

  public LynxBaseUI getDrawParent() {
    return (LynxBaseUI) mDrawParent;
  }

  public UIParent getParent() {
    return mParent;
  }

  public void insertChild(LynxBaseUI child, int index) {
    mChildren.add(index, child);
    if (mStableChildrenForTranslateZ != null) {
      mStableChildrenForTranslateZ.add(index, child);
    }
    child.setParent(this);
  }

  public void flattenChildrenCountIncrement() {
    ++mFlattenChildrenCount;
  }

  public void removeChild(LynxBaseUI child) {
    mChildren.remove(child);
    if (mStableChildrenForTranslateZ != null) {
      mStableChildrenForTranslateZ.remove(child);
    }
    child.setParent(null);
  }

  public void flattenChildrenCountDecrement() {
    --mFlattenChildrenCount;
  }

  public int flattenChildrenCount() {
    return mFlattenChildrenCount;
  }

  public int getIndex(LynxBaseUI child) {
    return mChildren.indexOf(child);
  }

  public List<LynxBaseUI> getChildren() {
    return mChildren;
  }

  public LynxBaseUI getChildAt(int index) {
    return mChildren.get(index);
  }

  public JavaOnlyMap getProps() {
    return mProps;
  }

  public float getTranslationX() {
    return 0;
  }

  public float getTranslationY() {
    return 0;
  }

  public float getTranslationZ() {
    return mTranslationZ;
  }

  // This API is used to obtain the real-time translateZ of UI. If the translateZ of UI is being
  // animated, the real-time value during the animation process will be obtained. Subclasses of
  // LynxBaseUI need to override this API and return the real-time translateZ.
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  protected float getRealTimeTranslationZ() {
    return mTranslationZ;
  }

  public void setTranslationZ(float zValue) {
    mTranslationZ = zValue;
  }

  public boolean getClipToRadius() {
    return mClipToRadius;
  }

  public final void updateProperties(StylesDiffMap props) {
    updatePropertiesInterval(props);
    afterPropsUpdated(props);
  }

  public void updatePropertiesInterval(StylesDiffMap props) {
    mProps.merge(props.mBackingMap);

    PropsUpdater.updateProps(this, props);
  }

  public void afterPropsUpdated(StylesDiffMap props) {
    onPropsUpdated();
    onAnimationUpdated();
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null) {
      wrapper.handleMutationStyleUpdate(this, props);
    }
  }

  @LynxUIMethod
  public void boundingClientRect(ReadableMap params, Callback callback) {
    // If ui owner is null, exec boundingClientRectInner directly.
    LynxUIOwner owner = mContext.getLynxUIOwner();
    if (owner == null) {
      boundingClientRectInner(params, callback);
      return;
    }

    // If rootUI is null, exec boundingClientRectInner directly.
    UIBody rootUI = owner.getRootUI();
    if (rootUI == null) {
      boundingClientRectInner(params, callback);
      return;
    }

    // If rootView is null, exec boundingClientRectInner directly.
    View rootView = rootUI.getView();
    if (rootView == null) {
      boundingClientRectInner(params, callback);
      return;
    }

    // If rootView.isLayoutRequested() == false, which means we are not in a layout process, then
    // exec boundingClientRectInner directly.
    if (!rootView.isLayoutRequested()) {
      boundingClientRectInner(params, callback);
      return;
    }

    // If the rootView.isLayoutRequested() == true, which means we are in a layout process, then
    // cache the ui and callback. Exec the callbacks after LynxUIOwner performLayout, then we can
    // get right layout info.
    mContext.getLynxUIOwner().registerBoundingClientRectUI(this);
    mBoundingClientRectCallbacks.add(new Runnable() {
      @Override
      public void run() {
        boundingClientRectInner(params, callback);
      }
    });
  }

  private void boundingClientRectInner(ReadableMap params, Callback callback) {
    RectF rect = LynxUIHelper.getRelativePositionInfo(this, params);

    float density = getLynxContext().getScreenMetrics().density;

    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("id", getIdSelector());
    result.putMap("dataset", (WritableMap) getDataset());
    result.putDouble("left", rect.left / density);
    result.putDouble("top", rect.top / density);
    result.putDouble("right", rect.right / density);
    result.putDouble("bottom", rect.bottom / density);
    result.putDouble("width", rect.width() / density);
    result.putDouble("height", rect.height() / density);

    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  public void uiOwnerDidPerformLayout() {
    if (mBoundingClientRectCallbacks.isEmpty()) {
      return;
    }
    ArrayList<Runnable> copyBoundingClientRectCallbacks =
        new ArrayList<>(mBoundingClientRectCallbacks);
    mBoundingClientRectCallbacks.clear();
    for (Runnable runnable : copyBoundingClientRectCallbacks) {
      runnable.run();
    }
  }

  @LynxUIMethod
  public void requestUIInfo(ReadableMap params, Callback callback) {
    ArrayList<String> fields = new ArrayList<>();
    if (params != null) {
      ReadableMapKeySetIterator it = params.keySetIterator();
      while (it.hasNextKey()) {
        String key = it.nextKey();
        if (params.getBoolean(key, false)) {
          fields.add(key);
        }
      }
    }

    // If params == null, getPositionInfo(false); If
    // params.PropsConstants.ANDROID_ENABLE_TRANSFORM_PROPS == false or params doesn't contain
    // PropsConstants.ANDROID_ENABLE_TRANSFORM_PROPS, getPositionInfo(false);
    JavaOnlyMap positionInfo = getPositionInfo(
        params != null && params.getBoolean(PropsConstants.ANDROID_ENABLE_TRANSFORM_PROPS, false));
    JavaOnlyMap result = new JavaOnlyMap();
    if (fields.contains("id")) {
      result.put("id", getIdSelector());
    }
    if (fields.contains("dataset")) {
      result.put("dataset", getDataset());
    }
    if (fields.contains("rect")) {
      // Same as boundingClientRect query callback
      result.put("left", positionInfo.get("left"));
      result.put("top", positionInfo.get("top"));
      result.put("right", positionInfo.get("right"));
      result.put("bottom", positionInfo.get("bottom"));
    }
    if (fields.contains("size")) {
      result.put("width", positionInfo.get("width"));
      result.put("height", positionInfo.get("height"));
    }
    // The node selected is <scroll-view> to get the position of scroll vertical and scroll
    // landscape. Otherwise, the two scrolling values will always be 0,0.
    if (fields.contains("scrollOffset")) {
      float density = getLynxContext().getScreenMetrics().density;
      result.put("scrollLeft", getScrollX() / density);
      result.put("scrollTop", getScrollY() / density);
    }
    if (fields.contains("node")) {
      // node: will overwrite in subclass.
      result.put("node", new JavaOnlyMap());
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  // If enbaleTransformProps, use LynxUIHelper.convertRectFromUIToRootUI rather than
  // getBoundingClientRect.
  // TODO(songshourui.null): Corresponding unit tests, checklists, and E2E tests will be added
  // later.
  private JavaOnlyMap getPositionInfo(boolean enableTransformProps) {
    RectF rect = new RectF();
    if (enableTransformProps) {
      rect = LynxUIHelper.convertRectFromUIToRootUI(this, new RectF(0, 0, getWidth(), getHeight()));
    } else {
      rect = new RectF(getBoundingClientRect());
    }

    float density = getLynxContext().getScreenMetrics().density;

    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("id", getIdSelector());
    result.putMap("dataset", (WritableMap) getDataset());
    result.putDouble("left", rect.left / density);
    result.putDouble("top", rect.top / density);
    result.putDouble("right", rect.right / density);
    result.putDouble("bottom", rect.bottom / density);
    result.putDouble("width", rect.width() / density);
    result.putDouble("height", rect.height() / density);
    return result;
  }

  /**
   * @deprecated this method will be removed in the future. Use {@link #scrollIntoView(ReadableMap,
   * Callback)} instead.
   */
  @Deprecated()
  public void scrollIntoView(ReadableMap params) {
    scrollIntoView(params, null);
  }

  @LynxUIMethod
  public void scrollIntoView(ReadableMap params, Callback callback) {
    if (params == null) {
      if (callback != null) {
        callback.invoke(
            LynxUIMethodConstants.PARAM_INVALID, "missing the param of `scrollIntoViewOptions`");
      }
      return;
    }
    HashMap<String, Object> paramsMap = params.asHashMap();
    HashMap<String, String> scrollIntoViewOptions =
        (HashMap<String, String>) paramsMap.get("scrollIntoViewOptions");
    if (scrollIntoViewOptions == null) {
      callback.invoke(
          LynxUIMethodConstants.PARAM_INVALID, "missing the param of `scrollIntoViewOptions`");
      return;
    }
    String behavior = scrollIntoViewOptions.containsKey("behavior")
        ? scrollIntoViewOptions.get("behavior")
        : "auto";
    String block =
        scrollIntoViewOptions.containsKey("block") ? scrollIntoViewOptions.get("block") : "start";
    String inline = scrollIntoViewOptions.containsKey("inline")
        ? scrollIntoViewOptions.get("inline")
        : "nearest";

    scrollIntoView(behavior.equals("smooth"), block, inline, callback);
  }

  /**
   * @deprecated this method will be removed in the future. Use {@link #scrollIntoView(boolean,
   * String, String, Callback)} instead.
   */
  @Deprecated()
  public void scrollIntoView(boolean isSmooth, String block, String inline) {
    scrollIntoView(isSmooth, block, inline, null);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void scrollIntoView(boolean isSmooth, String block, String inline, Callback callback) {
    Boolean scrollFlag = false;
    LynxBaseUI node = (LynxBaseUI) this.getParent();
    while (node != null && node instanceof LynxBaseUI) {
      if (node instanceof AbsLynxUIScroll) {
        ((AbsLynxUIScroll) node).scrollInto(this, isSmooth, block, inline);
        scrollFlag = true;
        break;
      }
      node = (LynxBaseUI) node.getParent();
    }
    if (!scrollFlag) {
      LLog.e(TAG, "scrollIntoView failed for nodeId:" + this.getSign());
      if (callback != null) {
        callback.invoke(LynxUIMethodConstants.OPERATION_ERROR,
            "scrollIntoView failed for nodeId:" + this.getSign());
      }
    } else {
      if (callback != null) {
        callback.invoke(LynxUIMethodConstants.SUCCESS);
      }
    }
  }

  @LynxProp(name = PropsConstants.FOCUSABLE)
  public void setFocusable(Boolean focusable) {
    mFocusable = (focusable != null ? focusable : false);
  }

  @LynxProp(name = PropsConstants.IGNORE_FOCUS)
  public void setIgnoreFocus(@Nullable Dynamic ignoreFocus) {
    // If ignoreFocus is null or not boolean, the mIgnoreFocus will be Undefined.
    if (ignoreFocus == null) {
      mIgnoreFocus = EnableStatus.Undefined;
      return;
    }
    try {
      mIgnoreFocus = ignoreFocus.asBoolean() ? EnableStatus.Enable : EnableStatus.Disable;
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      mIgnoreFocus = EnableStatus.Undefined;
    }
  }

  @LynxProp(name = PropsConstants.TEST_TAG)
  public void setTestID(@Nullable String tag) {
    mTestTagName = tag;
  }

  public String getTestID() {
    return mTestTagName == null ? "" : mTestTagName;
  }

  public Rect getBoundingClientRect() {
    ViewGroup rootView = mContext.getUIBody().getBodyView();
    int left = 0, top = 0;
    if (rootView == null) {
      return new Rect(left, top, left + getWidth(), top + getHeight());
    }
    if (this instanceof LynxUI) {
      View uiView = ((LynxUI) this).mView;
      // When box-shadow is applied, the original view will be
      // wrapped into UIShadowProxy.ShadowView whose coordinate
      // is always (0, 0).
      if (uiView instanceof UIShadowProxy.ShadowView
          && ((UIShadowProxy.ShadowView) uiView).getChildCount() > 0) {
        uiView = ((UIShadowProxy.ShadowView) uiView).getChildAt(0);
      }
      Rect offsetViewBounds = new Rect();
      View rootParent = uiView.getRootView();
      // maybe detached or x-overlay
      if (rootView.getRootView() != rootParent && rootParent instanceof ViewGroup) {
        rootView = (ViewGroup) rootParent;
      }
      try {
        rootView.offsetDescendantRectToMyCoords(uiView, offsetViewBounds);
        offsetViewBounds.offset(uiView.getScrollX(), uiView.getScrollY());
      } catch (IllegalArgumentException e) {
        // when LynxBaseUI is detached,
        // `offsetDescendantRectToMyCoords` throw exception.
        // It's safe to ignore this exception.
      }
      int[] offset = getOffsetDescendantRectToLynxView();
      if (offset[0] != Integer.MIN_VALUE) {
        offsetViewBounds.offset(offset[0], offset[1]);
      }
      top = offsetViewBounds.top;
      left = offsetViewBounds.left;
    } else if (this instanceof LynxFlattenUI) {
      if (mParent != null && mParent != mContext.getUIBody()) {
        LynxBaseUI current = this;
        while (current instanceof LynxFlattenUI && current != mContext.getUIBody()) {
          left += current.getOriginLeft();
          top += current.getOriginTop();
          current = current.getParentBaseUI();
        }
        if (current != null) {
          Rect other = current.getBoundingClientRect();
          left += other.left - current.getScrollX();
          top += other.top - current.getScrollY();
        }
      } else {
        left = mLeft;
        top = mTop;
      }
    }
    return new Rect(left, top, left + getWidth(), top + getHeight());
  }

  public void transformFromViewToRootView(View fromView, float[] inOutLocation) {
    if (!fromView.getMatrix().isIdentity()) {
      fromView.getMatrix().mapPoints(inOutLocation);
    }

    View root_view = fromView.getRootView();
    View current_view = fromView;

    while (current_view != root_view) {
      final View parent_view = (View) current_view.getParent();
      if (parent_view == null) {
        LLog.e(TAG, "transformFromViewToRootView failed, parent is null.");
        break;
      }

      inOutLocation[0] += current_view.getLeft();
      inOutLocation[1] += current_view.getTop();

      inOutLocation[0] -= parent_view.getScrollX();
      inOutLocation[1] -= parent_view.getScrollY();

      if (!parent_view.getMatrix().isIdentity()) {
        parent_view.getMatrix().mapPoints(inOutLocation);
      }

      current_view = parent_view;
    }
  }

  public TransOffset getTransformValue(float left, float right, float top, float bottom) {
    TransOffset res = new TransOffset();
    res.left_top = getLocationOnScreen(new float[] {left, top});
    res.right_top = getLocationOnScreen(new float[] {mWidth + right, top});
    res.right_bottom = getLocationOnScreen(new float[] {mWidth + right, mHeight + bottom});
    res.left_bottom = getLocationOnScreen(new float[] {left, mHeight + bottom});
    return res;
  }

  public Rect getRectToWindow() {
    ViewGroup rootView = mContext.getUIBody().getBodyView();
    if (rootView == null) {
      return new Rect();
    }
    int[] outLocation = new int[2];
    rootView.getLocationOnScreen(outLocation);
    Rect localRect = getBoundingClientRect();
    localRect.offset(outLocation[0], outLocation[1]);
    return localRect;
  }

  public float[] scrollBy(float width, float height) {
    float[] res = new float[4];
    res[0] = 0;
    res[1] = 0;
    res[2] = width;
    res[3] = height;
    return res;
  }

  @LynxProp(name = PropsConstants.NAME)
  public void setName(@Nullable String name) {
    mName = name;
  }

  @LynxProp(name = PropsConstants.ID_SELECTOR)
  public void setIdSelector(@Nullable String idSelector) {
    mIdSelector = idSelector;
  }

  @LynxProp(name = PropsConstants.REACT_REF_ID)
  public void setRefIdSelector(@Nullable String refId) {
    mRefId = refId;
  }

  @LynxProp(name = PropsConstants.DATASET)
  public void setDataset(@Nullable ReadableMap dataset) {
    mDataset = dataset;
  }

  @LynxProp(name = PropsConstants.BACKGROUND_COLOR, defaultInt = Color.TRANSPARENT)
  public void setBackgroundColor(int backgroundColor) {
    mBackgroundColor = backgroundColor;
    if (getKeyframeManager() != null) {
      getKeyframeManager().notifyPropertyUpdated(
          LynxKeyframeAnimator.sBackgroundColorStr, backgroundColor);
    }
    if (getTransitionAnimator() != null
        && getTransitionAnimator().containTransition(AnimationConstant.PROP_BACKGROUND_COLOR)) {
      getTransitionAnimator().applyPropertyTransition(
          this, AnimationConstant.PROP_BACKGROUND_COLOR, backgroundColor);
    } else {
      mLynxBackground.setBackgroundColor(backgroundColor);
      invalidate();
    }
  }

  @LynxProp(name = PropsConstants.CONSUME_SLIDE_EVENT)
  public void setConsumeSlideEvent(@Nullable ReadableArray array) {
    // If the array is null, then let mConsumeSlideEventAngles be null. If the array is not null,
    // check each item of the array to see if it is an array and the first two items are numbers. If
    // it meets the conditions, put it into mConsumeSlideEventAngles. Otherwise, skip the item. If
    // mConsumeSlideEventAngles is not empty, markNeedCheckConsumeSlideEvent is executed to indicate
    // that consumeSlideEvent detection is needed.
    if (array == null) {
      mConsumeSlideEventAngles = null;
      return;
    }
    try {
      ArrayList<Object> arrayList = array.toArrayList();
      mConsumeSlideEventAngles = new ArrayList<>();
      for (Object obj : arrayList) {
        if (obj instanceof ArrayList && ((ArrayList) obj).size() == 2
            && ((ArrayList) obj).get(0) instanceof Number
            && ((ArrayList) obj).get(1) instanceof Number) {
          ArrayList<Float> ary = new ArrayList<>();
          ary.add(((Number) ((ArrayList) obj).get(0)).floatValue());
          ary.add(((Number) ((ArrayList) obj).get(1)).floatValue());
          mConsumeSlideEventAngles.add(ary);
        }
      }
    } catch (Throwable e) {
      // Add a try-catch block to prevent exceptions thrown by `toArrayList()`.
      LLog.e(TAG, "setConsumeSlideEvent failed since " + e.getMessage());
    }
  }

  @LynxProp(name = "block-list-event", defaultBoolean = false)
  public void setBlockListEvent(boolean blockListEvent) {
    this.mBlockListEvent = blockListEvent;
  }

  // TODO(songshourui.null): remove me.
  // There are some bad cases that image-config can't be set as RGB_565,
  // e.g. meizu15 which's Build.VERSION is N_MR1.
  private boolean isImageConfigBadCase() {
    return Build.VERSION.SDK_INT == Build.VERSION_CODES.N_MR1 && isMeizu15();
  }

  private static boolean isMeizu() {
    String brand = Build.BRAND;
    if (brand == null) {
      return false;
    }
    return brand.toLowerCase(Locale.ENGLISH).indexOf("meizu") > -1;
  }

  private static boolean isMeizu15() {
    if (isMeizu() && !TextUtils.isEmpty(Build.DEVICE)) {
      return Build.DEVICE.contains("15");
    }
    return false;
  }

  private LynxMask getOrCreateLynxMask() {
    if (mLynxMask == null) {
      mLynxMask = new LynxMask(mContext);
      mLynxMask.setDrawableCallback(mDrawableCallback);
      mLynxMask.setFontSize(mFontSize);
      mLynxMask.setEnableBitmapGradient(mEnableBitmapGradient);
      mLynxMask.setBitmapConfig(mBitmapConfig);
      mLynxMask.setBorderWidth(SPACING_TYPES[mBorderSpacingIndex], mBorderWidth);
      mLynxMask.updatePaddingWidths(mPaddingTop, mPaddingRight, mPaddingBottom, mPaddingLeft);
    }
    return mLynxMask;
  }

  @LynxProp(name = PropsConstants.IAMGE_CONFIG)
  public void setImageConfig(@Nullable String config) {
    if (config == null || config.equalsIgnoreCase("")) {
      mBitmapConfig = null;
      return;
    }
    if (config.equalsIgnoreCase("ALPHA_8")) {
      mBitmapConfig = Bitmap.Config.ALPHA_8;
    } else if (config.equalsIgnoreCase("RGB_565")) {
      if (!isImageConfigBadCase()) {
        mBitmapConfig = Bitmap.Config.RGB_565;
      } else {
        LLog.w("LynxBaseUI setImageConfig warn: ", "RGB_565 can't be set on Meizu15");
        mBitmapConfig = null;
      }
    } else if (config.equalsIgnoreCase("ARGB_8888")) {
      mBitmapConfig = Bitmap.Config.ARGB_8888;
    } else if (config.equalsIgnoreCase("RGBA_F16")) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        mBitmapConfig = Bitmap.Config.RGBA_F16;
      } else {
        LLog.w("LynxBaseUI setImageConfig warn: ",
            "RGBA_F16 requires build version >= VERSION_CODES.O");
        mBitmapConfig = null;
      }
    } else if (config.equalsIgnoreCase("HARDWARE")) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        mBitmapConfig = Bitmap.Config.HARDWARE;
      } else {
        mBitmapConfig = null;
        LLog.w("LynxBaseUI setImageConfig warn: ",
            "HARDWARE requires build version >= VERSION_CODES.O");
      }
    } else {
      mBitmapConfig = null;
    }

    if (mLynxBackground != null) {
      mLynxBackground.setBitmapConfig(mBitmapConfig);
    }
    if (mLynxMask != null) {
      mLynxMask.setBitmapConfig(mBitmapConfig);
    }
  }

  @LynxProp(name = PropsConstants.IMAGE_RENDERING)
  public void setImageRendering(int imageRendering) {
    mImageRendering = imageRendering;
  }

  @LynxProp(name = PropsConstants.BITMAP_GRADIENT)
  public void setEnableBitmapGradient(boolean enable) {
    mEnableBitmapGradient = enable;
    mLynxBackground.setEnableBitmapGradient(enable);
    if (mLynxMask != null) {
      mLynxMask.setEnableBitmapGradient(enable);
    }
    invalidate();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_IMAGE)
  public void setBackgroundImage(@Nullable ReadableArray bgImage) {
    mLynxBackground.setLayerImage(bgImage, this);
    invalidate();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_ORIGIN)
  public void setBackgroundOrigin(@Nullable ReadableArray bgOrigin) {
    mLynxBackground.setLayerOrigin(bgOrigin);
    invalidate();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_POSITION)
  public void setBackgroundPosition(@Nullable ReadableArray bgPosition) {
    mLynxBackground.setLayerPosition(bgPosition);
    invalidate();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_REPEAT)
  public void setBackgroundRepeat(@Nullable ReadableArray bgRepeat) {
    mLynxBackground.setLayerRepeat(bgRepeat);
    invalidate();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_SIZE)
  public void setBackgroundSize(@Nullable ReadableArray bgSize) {
    mLynxBackground.setLayerSize(bgSize);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_IMAGE)
  public void setMaskImage(@Nullable ReadableArray maskImage) {
    getOrCreateLynxMask().setLayerImage(maskImage, this);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_ORIGIN)
  public void setMaskOrigin(@Nullable ReadableArray maskOrigin) {
    getOrCreateLynxMask().setLayerOrigin(maskOrigin);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_POSITION)
  public void setMaskPosition(@Nullable ReadableArray maskPosition) {
    getOrCreateLynxMask().setLayerPosition(maskPosition);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_REPEAT)
  public void setMaskRepeat(@Nullable ReadableArray maskRepeat) {
    getOrCreateLynxMask().setLayerRepeat(maskRepeat);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_SIZE)
  public void setMaskSize(@Nullable ReadableArray maskSize) {
    getOrCreateLynxMask().setLayerSize(maskSize);
    invalidate();
  }

  @LynxProp(name = PropsConstants.MASK_CLIP)
  public void setMaskClip(@Nullable ReadableArray maskClip) {
    getOrCreateLynxMask().setLayerClip(maskClip);
    invalidate();
  }

  @LynxProp(name = PropsConstants.BOX_SHADOW)
  public void setBoxShadow(@Nullable ReadableArray shadow) {
    if (mParent instanceof UIShadowProxy) {
      UIShadowProxy uiShadowProxy = (UIShadowProxy) mParent;
      uiShadowProxy.setBoxShadow(shadow);
    }
  }

  @LynxProp(name = PropsConstants.EXPOSURE_ID)
  public void setExposureID(Dynamic exposureID) {
    // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
    // onPropsUpdated for processing.
    mContext.removeUIFromExposedMap(this);
    String id = exposureID.asString();
    if (id.isEmpty()) {
      id = null;
      mExposureID = id;
      String errMsg =
          "setExposureID(Dynamic exposureID) failed, since it is not number/string, or it is empty string";
      LLog.e("LynxBaseUI", errMsg);
      LLog.DTHROW(new RuntimeException(errMsg));
    } else {
      mExposureID = id;
    }
  }

  public String getExposureID() {
    return mExposureID;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_SCENE)
  public void setExposureScene(String exposureScene) {
    // TODO(hexionghui): Put removeUIFromExposedMap caused by exposure-related properties in
    // onPropsUpdated for processing.
    mContext.removeUIFromExposedMap(this);
    mExposureScene = exposureScene;
  }

  public String getExposureScene() {
    return mExposureScene;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_SCREEN_MARGIN_TOP)
  public void setExposureScreenMarginTop(String exposureScreenMarginTop) {
    mExposureScreenMarginTop = UnitUtils.toPx(exposureScreenMarginTop);
  }

  public float getExposureScreenMarginTop() {
    return mExposureScreenMarginTop;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_SCREEN_MARGIN_BOTTOM)
  public void setExposureScreenMarginBottom(String exposureScreenMarginBottom) {
    mExposureScreenMarginBottom = UnitUtils.toPx(exposureScreenMarginBottom);
  }

  public float getExposureScreenMarginBottom() {
    return mExposureScreenMarginBottom;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_SCREEN_MARGIN_LEFT)
  public void setExposureScreenMarginLeft(String exposureScreenMarginLeft) {
    mExposureScreenMarginLeft = UnitUtils.toPx(exposureScreenMarginLeft);
  }

  public float getExposureScreenMarginLeft() {
    return mExposureScreenMarginLeft;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_SCREEN_MARGIN_RIGHT)
  public void setExposureScreenMarginRight(String exposureScreenMarginRight) {
    mExposureScreenMarginRight = UnitUtils.toPx(exposureScreenMarginRight);
  }

  public float getExposureScreenMarginRight() {
    return mExposureScreenMarginRight;
  }

  @LynxProp(name = PropsConstants.ENABLE_EXPOSURE_UI_MARGIN)
  public void setEnableExposureUIMargin(boolean enableExposureUIMargin) {
    mEnableExposureUIMargin = enableExposureUIMargin;
  }

  public boolean getEnableExposureUIMargin() {
    return mEnableExposureUIMargin != null ? mEnableExposureUIMargin
                                           : mContext.getEnableExposureUIMargin();
  }

  @LynxProp(name = PropsConstants.EXPOSURE_UI_MARGIN_TOP)
  public void setExposureUIMarginTop(String exposureUIMarginTop) {
    mExposureUIMarginTop = exposureUIMarginTop;
  }

  public String getExposureUIMarginTop() {
    return mExposureUIMarginTop;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_UI_MARGIN_BOTTOM)
  public void setExposureUIMarginBottom(String exposureUIMarginBottom) {
    mExposureUIMarginBottom = exposureUIMarginBottom;
  }

  public String getExposureUIMarginBottom() {
    return mExposureUIMarginBottom;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_UI_MARGIN_LEFT)
  public void setExposureUIMarginLeft(String exposureUIMarginLeft) {
    mExposureUIMarginLeft = exposureUIMarginLeft;
  }

  public String getExposureUIMarginLeft() {
    return mExposureUIMarginLeft;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_UI_MARGIN_RIGHT)
  public void setExposureUIMarginRight(String exposureUIMarginRight) {
    mExposureUIMarginRight = exposureUIMarginRight;
  }

  public String getExposureUIMarginRight() {
    return mExposureUIMarginRight;
  }

  @LynxProp(name = PropsConstants.EXPOSURE_AREA)
  public void setExposureArea(String exposureArea) {
    mExposureArea = exposureArea;
  }

  public String getExposureArea() {
    return mExposureArea;
  }

  @Deprecated
  public void setBorderRadius(int index, @Nullable String borderRadius) {
    LLog.DTHROW(
        new RuntimeException("setBorderWidth(int, String) is deprecated.This has no effect."));
  }

  @Deprecated
  public void setBorderWidth(int index, String borderWidth) {
    LLog.DTHROW(
        new RuntimeException("setBorderWidth(int, String) is deprecated.This has no effect."));
  }

  @Deprecated
  public void setBorderColor(@Nullable String borderColor) {
    LLog.DTHROW(new RuntimeException("setBorderColor(String) is deprecated.This has no effect."));
  }

  /**
   * when sub components need to know border-radius updated, can override this func.
   * index means: 0:top 1:right 2:bottom 3:left
   */
  public void onBorderRadiusUpdated(int index) {}

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_RADIUS,
                         PropsConstants.BORDER_TOP_LEFT_RADIUS,
                         PropsConstants.BORDER_TOP_RIGHT_RADIUS,
                         PropsConstants.BORDER_BOTTOM_RIGHT_RADIUS,
                         PropsConstants.BORDER_BOTTOM_LEFT_RADIUS,
                     })
  public void
  setBorderRadius(int index, @Nullable ReadableArray ra) {
    mHasRadius = false;
    if (mLynxBackground.setBorderRadius(index, ra)) {
      mHasRadius = true;
    }
    onBorderRadiusUpdated(index);
  }

  public int getInitialOverflowType() {
    return StyleConstants.OVERFLOW_HIDDEN;
  }

  @LynxProp(name = PropsConstants.OVERFLOW)
  public void setOverflow(@Nullable Integer overflow) {
    int value;
    if (overflow == null) {
      value = getInitialOverflowType();
    } else {
      value = overflow.intValue();
    }
    setOverflow(value);
  }

  public void setOverflow(int overflow) {
    setOverflowWithMask(OVERFLOW_XY, overflow);
  }

  @LynxProp(name = PropsConstants.OVERFLOW_X)
  public void setOverflowX(@Nullable Integer overflowX) {
    int value;
    if (overflowX == null) {
      value = getInitialOverflowType();
    } else {
      value = overflowX.intValue();
    }
    setOverflowWithMask(OVERFLOW_X, value);
  }

  @LynxProp(name = PropsConstants.OVERFLOW_Y)
  public void setOverflowY(@Nullable Integer overflowY) {
    int value;
    if (overflowY == null) {
      value = getInitialOverflowType();
    } else {
      value = overflowY.intValue();
    }
    setOverflowWithMask(OVERFLOW_Y, value);
  }

  @LynxProp(name = PropsConstants.USER_INTERACTION_ENABLED, defaultBoolean = true)
  public void setUserInteractionEnabled(@Nullable boolean userInteractionEnabled) {
    this.userInteractionEnabled = userInteractionEnabled;
  }

  @LynxProp(name = PropsConstants.NATIVE_INTERACTION_ENABLED, defaultBoolean = false)
  public void setNativeInteractionEnabled(@Nullable boolean nativeInteractionEnabled) {
    this.nativeInteractionEnabled = nativeInteractionEnabled;
  }

  @LynxProp(name = PropsConstants.BLOCK_NATIVE_EVENT, defaultBoolean = false)
  public void setBlockNativeEvent(@Nullable boolean blockNativeEvent) {
    this.mBlockNativeEvent = blockNativeEvent;
  }

  @LynxProp(name = PropsConstants.BLOCK_NATIVE_EVENT_AREAS)
  public void setBlockNativeEventAreas(@Nullable Dynamic maybeBlockNativeEventAreas) {
    this.mBlockNativeEventAreas = null;
    if (maybeBlockNativeEventAreas == null
        || maybeBlockNativeEventAreas.getType() != ReadableType.Array
        || maybeBlockNativeEventAreas.asArray() == null) {
      LLog.w(TAG, "setBlockNativeEventAreas input type error");
      return;
    }
    ReadableArray blockNativeEventAreas = maybeBlockNativeEventAreas.asArray();
    // Supports two types: `30px` and `50%`
    ArrayList<ArrayList<SizeValue>> areas = new ArrayList<ArrayList<SizeValue>>();
    for (int i = 0; i < blockNativeEventAreas.size(); ++i) {
      ReadableArray area = blockNativeEventAreas.getArray(i);
      if (area == null || area.size() != 4) {
        LLog.w(TAG, "setBlockNativeEventAreas " + i + "th type error, size != 4");
        continue;
      }
      SizeValue x = SizeValue.fromCSSString(area.getString(0));
      SizeValue y = SizeValue.fromCSSString(area.getString(1));
      SizeValue w = SizeValue.fromCSSString(area.getString(2));
      SizeValue h = SizeValue.fromCSSString(area.getString(3));
      if (x != null && y != null && w != null && h != null) {
        ArrayList<SizeValue> values = new ArrayList<SizeValue>();
        values.add(x);
        values.add(y);
        values.add(w);
        values.add(h);
        areas.add(values);
      } else {
        LLog.w(TAG, "setBlockNativeEventAreas " + i + "th type error");
      }
    }
    if (areas.size() > 0) {
      this.mBlockNativeEventAreas = areas;
    } else {
      LLog.w(TAG, "setBlockNativeEventAreas empty areas");
    }
  }

  @LynxProp(name = PropsConstants.EVENT_THROUGH)
  public void setEventThrough(@Nullable Dynamic eventThrough) {
    // If eventThrough is null or not boolean, the mEventThrough will be Undefined.
    if (eventThrough == null) {
      mEventThrough = EnableStatus.Undefined;
      return;
    }
    try {
      mEventThrough = eventThrough.asBoolean() ? EnableStatus.Enable : EnableStatus.Disable;
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      mEventThrough = EnableStatus.Undefined;
    }
  }

  @LynxProp(name = PropsConstants.ENABLE_TOUCH_PSEUDO_PROPAGATION)
  public void setEnableTouchPseudoPropagation(@Nullable Dynamic enableTouchPseudoPropagation) {
    // If enableTouchPseudoPropagation is null or not boolean, the mEnableTouchPseudoPropagation
    // will be true.
    if (enableTouchPseudoPropagation == null) {
      this.mEnableTouchPseudoPropagation = true;
      return;
    }
    try {
      this.mEnableTouchPseudoPropagation = enableTouchPseudoPropagation.asBoolean();
    } catch (Throwable e) {
      LLog.i(TAG, e.toString());
      this.mEnableTouchPseudoPropagation = true;
    }
  }

  protected void setOverflowWithMask(final short mask, @Nullable int overflow) {
    int newVal = mOverflow;
    if (overflow == OVERFLOW_VISIBLE) {
      newVal |= mask;
    } else {
      newVal &= ~mask;
    }
    mOverflow = newVal;
    requestLayout();
  }

  public int getOverflow() {
    return mOverflow;
  }

  protected Rect getBoundRectForOverflow() {
    if (getOverflow() == OVERFLOW_XY) {
      return null;
    }
    return getClipBounds();
  }

  protected Rect getClipBounds() {
    int w = getWidth(), h = getHeight();
    int x = 0, y = 0;
    DisplayMetrics dm = getLynxContext().getScreenMetrics();
    if ((getOverflow() & OVERFLOW_X) != 0) {
      x -= dm.widthPixels;
      w += 2 * dm.widthPixels;
    }
    if ((getOverflow() & OVERFLOW_Y) != 0) {
      y -= dm.heightPixels;
      h += 2 * dm.heightPixels;
    }
    return new Rect(x, y, x + w, y + h);
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_STYLE,
                         PropsConstants.BORDER_LEFT_STYLE,
                         PropsConstants.BORDER_RIGHT_STYLE,
                         PropsConstants.BORDER_TOP_STYLE,
                         PropsConstants.BORDER_BOTTOM_STYLE,
                     },
      defaultInt = -1)
  public void
  setBorderStyle(int index, int borderStyle) {
    mLynxBackground.setBorderStyle(SPACING_TYPES[index], borderStyle);
  }

  private float toPix(String str) {
    UIBody uiBody = mContext.getUIBody();
    final float ret = UnitUtils.toPxWithDisplayMetrics(str, uiBody.getFontSize(), getFontSize(),
        uiBody.getWidth(), uiBody.getHeight(), MeasureUtils.UNDEFINED, mContext.getScreenMetrics());
    return ret;
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_WIDTH,
                         PropsConstants.BORDER_LEFT_WIDTH,
                         PropsConstants.BORDER_RIGHT_WIDTH,
                         PropsConstants.BORDER_TOP_WIDTH,
                         PropsConstants.BORDER_BOTTOM_WIDTH,
                     })
  public void
  setBorderWidth(int index, int borderWidth) {
    mBorderSpacingIndex = index;
    mBorderWidth = borderWidth;
    mLynxBackground.setBorderWidth(SPACING_TYPES[index], borderWidth);
    if (mLynxMask != null) {
      mLynxMask.setBorderWidth(SPACING_TYPES[index], borderWidth);
    }
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_LEFT_COLOR,
                         PropsConstants.BORDER_RIGHT_COLOR,
                         PropsConstants.BORDER_TOP_COLOR,
                         PropsConstants.BORDER_BOTTOM_COLOR,
                     },
      customType = "Color")
  public void
  setBorderColor(int index, Integer color) {
    mLynxBackground.setBorderColorForSpacingIndex(SPACING_TYPES[index + 1], color);
  }

  @LynxProp(name = PropsConstants.OUTLINE_COLOR, defaultInt = Color.BLACK)
  public void setOutlineColor(int color) {
    if (mParent instanceof UIShadowProxy) {
      UIShadowProxy uiShadowProxy = (UIShadowProxy) mParent;
      uiShadowProxy.setOutlineColor(color);
    }
  }

  @LynxProp(name = PropsConstants.OUTLINE_WIDTH, defaultInt = 0)
  public void setOutlineWidth(float outlineWidth) {
    if (mParent instanceof UIShadowProxy) {
      UIShadowProxy uiShadowProxy = (UIShadowProxy) mParent;
      uiShadowProxy.setOutlineWidth(outlineWidth);
    }
  }

  @LynxProp(name = PropsConstants.OUTLINE_STYLE, defaultInt = -1)
  public void setOutlineStyle(int outlineStyle) {
    if (mParent instanceof UIShadowProxy) {
      UIShadowProxy uiShadowProxy = (UIShadowProxy) mParent;
      uiShadowProxy.setOutlineStyle(BorderStyle.parse(outlineStyle));
    }
  }

  @LynxProp(name = PropsConstants.FONT_SIZE, defaultFloat = MeasureUtils.UNDEFINED)
  public void setFontSize(float fontSize) {
    if (fontSize != MeasureUtils.UNDEFINED) {
      mFontSize = fontSize;
      mLynxBackground.setFontSize(mFontSize);
      if (mLynxMask != null) {
        mLynxMask.setFontSize(mFontSize);
      }
    }
  }

  @LynxProp(name = PropsConstants.BACKGROUND_CLIP)
  public void setBackgroundClip(@Nullable ReadableArray bgClip) {
    mLynxBackground.setLayerClip(bgClip);
  }

  @LynxProp(name = PropsConstants.CLIP_TO_RADIUS)
  public void setClipToRadius(@Nullable Dynamic clip) {
    if (clip == null) {
      return;
    }
    ReadableType type = clip.getType();
    if (type == ReadableType.Boolean) {
      mClipToRadius = clip.asBoolean();
    } else if (type == ReadableType.String) {
      String clipStr = clip.asString();
      mClipToRadius = clipStr.equalsIgnoreCase("true") || clipStr.equalsIgnoreCase("yes");
    }
  }

  @Deprecated
  public void setBorderColor(Integer color) {
    setBorderColorForAllSpacingIndex(color);
  }

  @Deprecated
  private void setBorderColorForAllSpacingIndex(Integer color) {
    float rgbComponent = color == null ? MeasureUtils.UNDEFINED : (float) (color & 0x00FFFFFF);
    float alphaComponent = color == null ? MeasureUtils.UNDEFINED : (float) (color >>> 24);
    // do not use Spacing.ALL (must ignore START and END)
    for (int i = 1; i <= 4; ++i) {
      mLynxBackground.setBorderColor(SPACING_TYPES[i], rgbComponent, alphaComponent);
    }
  }

  @LynxProp(name = PropsConstants.CARET_COLOR)
  public void setCaretColor(@Nullable String caretColor) {
    // implemented by components
  }

  /**
   * ---------- Accessibility Section ----------
   */

  public boolean isAccessibilityHostUI() {
    return false;
  }

  public boolean isAccessibilityDirectionVertical() {
    return true;
  }

  public boolean requestChildUIRectangleOnScreen(LynxBaseUI child, Rect rect, boolean smooth) {
    return false;
  }

  @LynxProp(name = PropsConstants.ANDROID_ACCESSIBILITY_KEEP_FOCUSED)
  public void setAccessibilityKeepFocused(@Nullable Dynamic value) {
    boolean res = false;
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        res = Boolean.parseBoolean(value.asString());
      } else if (type == ReadableType.Boolean) {
        res = value.asBoolean();
      }
    }
    mAccessibilityKeepFocused = res;
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ELEMENTS)
  public void setAccessibilityElements(@Nullable Dynamic value) {
    if (value != null && value.getType() == ReadableType.String) {
      String[] elementIds = value.asString().split(",");
      if (elementIds.length > 0) {
        if (mAccessibilityElements == null) {
          mAccessibilityElements = new ArrayList<>();
        }
        mAccessibilityElements.clear();
        for (String elementId : elementIds) {
          mAccessibilityElements.add(elementId);
        }
        LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
        if (wrapper != null) {
          wrapper.addAccessibilityElementsUI(this);
        }
      }
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_LABEL)
  public void setAccessibilityLabel(@Nullable Dynamic value) {
    // accessibility-label="" -> label = null, set "" means we have set accessibility-label
    String label = "";
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        label = value.asString();
      } else if (type == ReadableType.Int || type == ReadableType.Number
          || type == ReadableType.Long) {
        label = String.valueOf(value.asInt());
      } else if (type == ReadableType.Boolean) {
        label = String.valueOf(value.asBoolean());
      }
    }
    mAccessibilityLabel = label;
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ELEMENT)
  public void setAccessibilityElement(@Nullable Dynamic value) {
    boolean b = true;
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        b = Boolean.parseBoolean(value.asString());
      } else if (type == ReadableType.Int || type == ReadableType.Number
          || type == ReadableType.Long) {
        b = value.asInt() != 0;
      } else if (type == ReadableType.Boolean) {
        b = value.asBoolean();
      }
    }
    mAccessibilityElementStatus = (b ? ACCESSIBILITY_ELEMENT_TRUE : ACCESSIBILITY_ELEMENT_FALSE);
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ENABLE_TAP)
  public void setAccessibilityEnableTap(@Nullable Dynamic value) {
    boolean res = false;
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        res = Boolean.parseBoolean(value.asString());
      } else if (type == ReadableType.Boolean) {
        res = value.asBoolean();
      }
    }
    mAccessibilityEnableTap = res;
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ELEMENTS_A11Y)
  public void setAccessibilityElementsA11y(@Nullable Dynamic value) {
    if (value != null && value.getType() == ReadableType.String) {
      String[] elementIds = value.asString().split(",");
      if (elementIds.length > 0) {
        if (mAccessibilityElementsA11y == null) {
          mAccessibilityElementsA11y = new ArrayList<>();
        }
        mAccessibilityElementsA11y.clear();
        for (String elementId : elementIds) {
          mAccessibilityElementsA11y.add(elementId);
        }
        LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
        if (wrapper != null) {
          wrapper.addAccessibilityElementsA11yUI(this);
        }
      }
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ID)
  public void setA11yId(@Nullable Dynamic value) {
    String accessibilityId = "";
    if (value != null && ReadableType.String == value.getType()) {
      accessibilityId = value.asString();
    }
    mAccessibilityId = accessibilityId;
  }

  /**
   * @name: accessibility-exclusive-focus
   * @description: When set to true, only accessible elements within the current subtree can be
   * focused.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = PropsConstants.ACCESSIBILITY_EXCLUSIVE_FOCUS, defaultBoolean = false)
  public void setAccessibilityExclusiveFocus(boolean value) {
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null) {
      wrapper.addOrRemoveUIFromExclusiveMap(this, value);
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_VALUE)
  public void setAccessibilityValue(String value) {}

  @LynxProp(name = PropsConstants.ACCESSIBILITY_HEADING)
  public void setAccessibilityHeading(boolean value) {}

  /**
   * @name: android-consume-hover-event
   * @description: Let the certain view consume hover event and not dispatch hover event to other
   * view.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = PropsConstants.CONSUME_HOVER_EVENT, defaultBoolean = false)
  public void setConsumeHoverEvent(boolean value) {
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null && wrapper.enableHelper()) {
      mConsumeHoverEvent = value;
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_TRAITS)
  public void setAccessibilityTraits(@Nullable Dynamic value) {
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        mAccessibilityTraits =
            LynxAccessibilityHelper.LynxAccessibilityTraits.fromValue(value.asString());
      }
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION)
  public void setAccessibilityRoleDescription(@Nullable Dynamic value) {
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        mAccessibilityRoleDescription = value.asString();
      }
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_STATUS)
  public void setAccessibilityStatus(@Nullable Dynamic value) {
    if (value != null) {
      ReadableType type = value.getType();
      if (type == ReadableType.String) {
        mAccessibilityStatus = value.asString();
      }
    }
  }

  @LynxProp(name = PropsConstants.ACCESSIBILITY_ACTIONS)
  public void setAccessibilityActions(ReadableArray array) {
    mAccessibilityActions = array.asArrayList();
  }

  @LynxUIMethod
  public void requestAccessibilityFocus(ReadableMap params, Callback callback) {
    if (mContext.getLynxAccessibilityWrapper() != null) {
      mContext.getLynxAccessibilityWrapper().requestAccessibilityFocus(this, callback);
    }
  }

  @LynxUIMethod
  public void fetchAccessibilityTargets(ReadableMap params, Callback callback) {
    if (mContext.getLynxAccessibilityWrapper() != null) {
      mContext.getLynxAccessibilityWrapper().fetchAccessibilityTargets(this, callback);
    }
  }

  @LynxUIMethod
  public void innerText(ReadableMap params, Callback callback) {
    if (mContext.getLynxAccessibilityWrapper() != null) {
      mContext.getLynxAccessibilityWrapper().innerText(this, callback);
    }
  }

  public String getAccessibilityId() {
    return mAccessibilityId;
  }

  public String getAccessibilityStatus() {
    return mAccessibilityStatus;
  }

  public ArrayList<String> getAccessibilityActions() {
    return mAccessibilityActions;
  }

  public ArrayList<String> getAccessibilityElementsA11y() {
    return mAccessibilityElementsA11y;
  }

  /**
   * ---------- Accessibility Section end ----------
   */

  @LynxProp(name = PropsConstants.POSITION, defaultFloat = StyleConstants.POSITION_RELATIVE)
  public final void setCSSPosition(int position) {
    mCSSPosition = position;
  }

  @LynxProp(name = PropsConstants.ENABLE_SCROLL_MONITOR)
  public void setEnableScrollMonitor(@Nullable boolean enable) {
    mEnableScrollMonitor = enable;
  }

  @LynxProp(name = PropsConstants.SCROLL_MONITOR_TAG)
  public void setScrollMonitorTag(@Nullable String scrollMonitorTag) {
    mScrollMonitorTag = scrollMonitorTag;
  }

  @LynxProp(name = PropsConstants.DRIECTION, defaultInt = StyleConstants.DIRECTION_LTR)
  public void setLynxDirection(int direction) {
    mLynxDirection = direction;
  }

  @LynxProp(name = PropsConstants.INTERSECTION_OBSERVERS)
  public void setIntersectionObservers(@Nullable ReadableArray observers) {
    mContext.getIntersectionObserverManager().removeAttachedIntersectionObserver(this);
    if (observers == null || !mEvents.containsKey("intersection")) {
      return;
    }
    for (int idx = 0; idx < observers.size(); idx++) {
      ReadableMap propsObject = observers.getMap(idx);
      if (propsObject != null) {
        LynxIntersectionObserver observer = new LynxIntersectionObserver(
            mContext.getIntersectionObserverManager(), propsObject, this);
        mContext.getIntersectionObserverManager().addIntersectionObserver(observer);
      }
    }
  }

  public JSONObject getPlatformCustomInfo() {
    return new JSONObject();
  }

  public int getCSSPositionType() {
    return mCSSPosition;
  }

  public ArrayList<String> getAccessibilityElements() {
    return mAccessibilityElements;
  }

  public CharSequence getAccessibilityLabel() {
    return mAccessibilityLabel;
  }

  public boolean getAccessibilityKeepFocused() {
    return mAccessibilityKeepFocused;
  }

  public int getAccessibilityElementStatus() {
    return mAccessibilityElementStatus;
  }

  public boolean getAccessibilityEnableTap() {
    return mAccessibilityEnableTap;
  }

  public LynxAccessibilityHelper.LynxAccessibilityTraits getAccessibilityTraits() {
    return mAccessibilityTraits;
  }

  public String getAccessibilityRoleDescription() {
    return mAccessibilityRoleDescription;
  }

  public String getName() {
    return mName;
  }

  public String getIdSelector() {
    return mIdSelector;
  }

  public String getRefIdSelector() {
    return mRefId;
  }

  @Override
  public ReadableMap getDataset() {
    return mDataset;
  }

  public Rect getBound() {
    return mBound;
  }

  public void setBound(Rect bound) {
    mBound = bound;
  }

  public int getWidth() {
    return mWidth;
  }

  public int getHeight() {
    return mHeight;
  }

  public int getTop() {
    return mTop;
  }

  public int getLeft() {
    return mLeft;
  }

  public int getPaddingLeft() {
    return mPaddingLeft;
  }

  public int getPaddingTop() {
    return mPaddingTop;
  }

  public int getPaddingRight() {
    return mPaddingRight;
  }

  public int getPaddingBottom() {
    return mPaddingBottom;
  }

  public int getBorderLeftWidth() {
    return mBorderLeftWidth;
  }

  public int getBorderTopWidth() {
    return mBorderTopWidth;
  }

  public int getBorderBottomWidth() {
    return mBorderBottomWidth;
  }

  public int getBorderRightWidth() {
    return mBorderRightWidth;
  }

  public int getMarginLeft() {
    return mMarginLeft;
  }

  public int getMarginTop() {
    return mMarginTop;
  }

  public int getMarginRight() {
    return mMarginRight;
  }

  public int getMarginBottom() {
    return mMarginBottom;
  }

  public int getScrollX() {
    return 0;
  }

  public int getScrollY() {
    return 0;
  }

  public float getFontSize() {
    return mFontSize;
  }

  public boolean getHasRadius() {
    return mHasRadius;
  }

  public List<TransformRaw> getTransformRaws() {
    return mTransformRaw;
  }

  public boolean isEnableScrollMonitor() {
    return mEnableScrollMonitor;
  }

  public String getScrollMonitorTag() {
    return mScrollMonitorTag;
  }

  public void setTransform(@Nullable ReadableArray transform) {
    hasTransformChanged = true;
    mTransformRaw = TransformRaw.toTransformRaw(transform);
    setTranslationZ(TransformRaw.hasZValue(mTransformRaw));
    if (getParent() instanceof UIShadowProxy) {
      ((UIShadowProxy) getParent()).updateTransform();
    }
  }

  @Override
  public boolean hasConsumeSlideEventAngles() {
    return mConsumeSlideEventAngles != null && !mConsumeSlideEventAngles.isEmpty();
  }

  @Nullable
  public TransformOrigin getTransformOriginStr() {
    return mTransformOrigin;
  }

  @LynxProp(name = PropsConstants.TRANSFORM_ORIGIN)
  public void setTransformOrigin(@Nullable ReadableArray params) {
    hasTransformChanged = true;
    mTransformOrigin = TransformOrigin.TRANSFORM_ORIGIN_DEFAULT;
    if (null == params) {
      return;
    }
    // TODO(liyanbo): this will replace by drawInfo. this is temp solution.
    // details can @see ComputedPlatformCSSStyle::TransformOriginToLepus
    mTransformOrigin = TransformOrigin.MakeTransformOrigin(params);
    if (null == mTransformOrigin) {
      LLog.e(TAG, "transform params error.");
      mTransformOrigin = TransformOrigin.TRANSFORM_ORIGIN_DEFAULT;
    }
  }

  @LynxProp(name = PropsConstants.PERSPECTIVE)
  public void setPerspective(@Nullable ReadableArray perspective) {
    mPerspective = perspective;
  }

  @LynxProp(name = PropsConstants.LOCAL_CACHE)
  public void setLocalCache(@Nullable Dynamic localCache) {
    mUseLocalCache = localCache;
  }

  @LynxProp(name = PropsConstants.SKIP_REDIRECTION)
  public void setSkipRedirection(boolean skipRedirection) {
    mSkipRedirection = skipRedirection;
  }

  public Dynamic getEnableLocalCache() {
    return mUseLocalCache;
  }

  public boolean getSkipRedirection() {
    return mSkipRedirection;
  }

  public int getImageRendering() {
    return mImageRendering;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public final boolean shouldDoTransform() {
    return hasTransformChanged
        || ((TransformRaw.hasPercent(mTransformRaw) || TransformOrigin.hasPercent(mTransformOrigin))
            && hasSizeChanged());
  }

  public void updateLayout(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    // During the updateLayoutInfo process, onLayoutUpdated is triggered,
    // but onLayoutUpdated is also triggered once more after updateLayoutInfo finishes.
    // Add a flag to avoid redundant triggers.
    mSkipLayoutUpdated = true;
    updateLayoutInfo(left, top, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
        marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth,
        borderRightWidth, borderBottomWidth, bound);
    mSkipLayoutUpdated = false;
    onLayoutUpdated();
    sendLayoutChangeEvent();
  }

  // the target Layout Info, only for transform percentage
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public final void updateLayoutSize(int width, int height) {
    mLatestSize.x = width;
    mLatestSize.y = height;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public final int getLatestWidth() {
    return mLatestSize.x;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public final int getLatestHeight() {
    return mLatestSize.y;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public final boolean hasSizeChanged() {
    return !mLastSize.equals(mLatestSize);
  }

  public void measure() {
    for (LynxBaseUI child : mChildren) {
      child.measure();
    }
  }

  public void layout() {
    for (LynxBaseUI child : mChildren) {
      child.layout();
    }
  }

  private void sendLayoutChangeEvent() {
    Map<String, EventsListener> events = mEvents;
    final String layoutChangeFunctionName = "layoutchange";
    if (events != null && events.containsKey(layoutChangeFunctionName)) {
      Map<String, Object> data =
          getPositionInfo(LynxEnv.inst().enableTransformForPositionCalculation());
      getLynxContext().getEventEmitter().sendCustomEvent(
          new LynxCustomEvent(getSign(), layoutChangeFunctionName, data));
    }
  }

  // compat
  @Deprecated
  public void updateLayout(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    updateLayoutInfo(left, top, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
        0, 0, 0, 0, borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound);
    onLayoutUpdated();
  }

  public int getOriginTop() {
    return mOriginTop;
  }

  public int getOriginLeft() {
    return mOriginLeft;
  }

  public void setOriginLeft(int originLeft) {
    mOriginLeft = originLeft;
  }

  public void setOriginTop(int originTop) {
    mOriginTop = originTop;
  }

  protected void updateLayoutInfo(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bound) {
    if (mContext != null && mContext.isTouchMoving() && Float.compare(mWidth, 0) != 0
        && Float.compare(mHeight, 0) != 0
        && (Float.compare(mLeft, left) != 0 || Float.compare(mTop, top) != 0)) {
      // The front-end modify the layout properties, causing the UI to slide.
      mContext.onPropsChanged(this);
    }
    setPosition(left, top);
    mWidth = width;
    mHeight = height;
    mPaddingLeft = paddingLeft;
    mPaddingRight = paddingRight;
    mPaddingBottom = paddingBottom;
    mPaddingTop = paddingTop;
    mMarginLeft = marginLeft;
    mMarginTop = marginTop;
    mMarginRight = marginRight;
    mMarginBottom = marginBottom;
    mBorderTopWidth = borderTopWidth;
    mBorderBottomWidth = borderBottomWidth;
    mBorderLeftWidth = borderLeftWidth;
    mBorderRightWidth = borderRightWidth;
    mBound = bound;
  }

  /**
   * Update position related props.
   */
  protected void setPosition(int left, int top) {
    mLeft = left;
    mTop = top;
    mOriginTop = top;
    mOriginLeft = left;
  }

  public void updateLayoutInfo(final LynxBaseUI layout) {
    updateLayoutInfo(layout.getLeft(), layout.getTop(), layout.getWidth(), layout.getHeight(),
        layout.getPaddingLeft(), layout.getPaddingTop(), layout.getPaddingRight(),
        layout.getPaddingBottom(), layout.getMarginLeft(), layout.getMarginTop(),
        layout.getMarginRight(), layout.getMarginBottom(), layout.getBorderLeftWidth(),
        layout.getBorderTopWidth(), layout.getBorderRightWidth(), layout.getBorderBottomWidth(),
        layout.getBound());
    mOriginLeft = layout.getOriginLeft();
    mOriginTop = layout.getOriginTop();
  }

  /**
   * Used in layout with LynxFlattenUI. Update the position for drawing. Don't change the native
   * layout value.
   *
   * @param left   relative left value to drawParent
   * @param top    relative top value to drawParent
   * @param bounds relative bounds value to drawParent
   */
  public boolean updateDrawingLayoutInfo(int left, int top, Rect bounds) {
    boolean changed = false;
    if (mLeft != left) {
      mLeft = left;
      changed = true;
    }
    if (mTop != top) {
      mTop = top;
      changed = true;
    }

    mBound = bounds;

    if (changed) {
      onDrawingPositionChanged();
    }
    return changed;
  }

  protected void onDrawingPositionChanged() {}

  public void setLeft(int left) {
    mLeft = left;
    mOriginLeft = left;
    onLayoutUpdated();
  }

  public void setTop(int top) {
    mTop = top;
    mOriginTop = top;
    onLayoutUpdated();
  }

  public void setWidth(int width) {
    mWidth = width;
    onLayoutUpdated();
  }

  public void setHeight(int height) {
    mHeight = height;
    onLayoutUpdated();
  }

  public void onLayoutUpdated() {
    if (mSkipLayoutUpdated) {
      return;
    }
    mLynxBackground.updatePaddingWidths(mPaddingTop, mPaddingRight, mPaddingBottom, mPaddingLeft);
    if (mLynxMask != null) {
      mLynxMask.updatePaddingWidths(mPaddingTop, mPaddingRight, mPaddingBottom, mPaddingLeft);
    }
    invalidate();
    requestLayout();
  }

  public void onPropsUpdated() {
    if (mContext != null) {
      mContext.addUIToExposedMap(this);
    }
    invalidate();
  }

  public void onBeforeAnimation(int left, int top, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom) {}

  /**
   * deprecated.
   * use {@link #onNodeReady()} instead.
   */
  @Deprecated
  public void onAnimationUpdated() {}

  /***
   *
   * @param operationId  the unique id for this action
   * @param component   the component view when calling renderComponentAtIndex()
   */
  public void onLayoutFinish(long operationId, @Nullable LynxBaseUI component) {}

  @Deprecated
  public void onLayoutFinish(long operationId) {}

  public void onAnimationNodeReady() {}

  public void afterAnimationNodeReady() {
    // Update some properties related to animation.
    if (mIsFirstAnimatedReady) {
      mIsFirstAnimatedReady = false;
    }
    mLastSize.x = mLatestSize.x;
    mLastSize.y = mLatestSize.y;
    hasTransformChanged = false;
  }

  // Override this interface if need to monitor node ready
  public void onNodeReady() {
    onAnimationNodeReady();
    afterAnimationNodeReady();
  }

  public void onNodeReload() {}

  @CallSuper
  public void onNodeRemoved() {}

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public boolean isFirstAnimatedReady() {
    return mIsFirstAnimatedReady;
  }

  public void initTransitionAnimator(ReadableMap map) {}

  public TransitionAnimationManager getTransitionAnimator() {
    return null;
  }

  @Nullable
  public KeyframeManager getKeyframeManager() {
    return null;
  }

  public boolean enableLayoutAnimation() {
    return false;
  }

  @Nullable
  public LayoutAnimationManager getLayoutAnimator() {
    return null;
  }

  public void setAnimation(@Nullable ReadableArray animationInfos) {}

  public void updateExtraData(final Object extraData) {}

  public void renderIfNeeded() {}

  public void onAttach() {
    mLynxBackground.onAttach();
    if (mLynxMask != null) {
      mLynxMask.onAttach();
    }
  }

  public void onDetach() {
    mLynxBackground.onDetach();
    if (mLynxMask != null) {
      mLynxMask.onDetach();
    }
  }

  private class DrawableCallback implements Drawable.Callback {
    @Override
    public void invalidateDrawable(@NonNull Drawable who) {
      LynxBaseUI.this.invalidate();
    }

    @Override
    public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {}

    @Override
    public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {}
  }

  protected float getScaleX() {
    return 1;
  }

  protected float getScaleY() {
    return 1;
  }

  protected Rect getRect() {
    // frame should calculate translate and scale
    float centerX = getOriginLeft() + getWidth() / 2.0f;
    float centerY = getOriginTop() + getHeight() / 2.0f;
    float width = getWidth() * getScaleX();
    float height = getHeight() * getScaleY();
    float rectX = centerX - width / 2.0f + getTranslationX();
    float rectY = centerY - height / 2.0f + getTranslationY();
    Rect transRect =
        new Rect((int) rectX, (int) rectY, (int) (rectX + width), (int) (rectY + height));

    if (transRect.width() + mHitSlopLeft + mHitSlopRight >= Float.MIN_VALUE
        && transRect.height() + mHitSlopTop + mHitSlopBottom >= Float.MIN_VALUE) {
      transRect.top -= mHitSlopTop;
      transRect.bottom += mHitSlopBottom;
      transRect.left -= mHitSlopLeft;
      transRect.right += mHitSlopRight;
    }

    return transRect;
  }

  // TODO(hexionghui): change the name to getHitTestRectWithoutTransform
  protected Rect getRectWithoutTransform() {
    float rectX = getOriginLeft();
    float rectY = getOriginTop();
    float width = getWidth();
    float height = getHeight();

    return new Rect((int) rectX, (int) rectY, (int) (rectX + width), (int) (rectY + height));
  }

  public LynxBaseUI getParentBaseUI() {
    if (mParent instanceof UIShadowProxy) {
      return (LynxBaseUI) ((UIShadowProxy) mParent).getParent();
    }
    return (LynxBaseUI) mParent;
  }

  public LynxBaseUI getExposeReceiveTarget() {
    return this;
  }

  public void removeChildrenExposureUI() {}

  public String getExposeUniqueID() {
    return String.valueOf(mSign);
  }

  public void updateSticky(float[] sticky) {
    if (sticky == null || sticky.length < 4) {
      mSticky = null;
      return;
    }

    mSticky = new Sticky();
    mSticky.left = sticky[0];
    mSticky.top = sticky[1];
    mSticky.right = sticky[2];
    mSticky.bottom = sticky[3];
    mSticky.x = mSticky.y = 0;

    LynxBaseUI parentBaseUI = getParentBaseUI();
    if (parentBaseUI instanceof IScrollSticky) {
      ((IScrollSticky) parentBaseUI).setEnableSticky();
    }
  }

  public void updateMaxHeight(float maxHeight) {
    mMaxHeight = maxHeight;
  }

  public boolean checkStickyOnParentScroll(int l, int t) {
    if (mSticky == null)
      return false;

    final float currentLeft = getLeft() - l;
    final float currentTop = getTop() - t;
    if (currentLeft < mSticky.left) {
      mSticky.x = mSticky.left - currentLeft;
    } else {
      if (getParentBaseUI() == null) {
        LLog.e(TAG, "checkStickyOnParentScroll failed, parent is null.");
        return false;
      }

      final int parentWidth = getParentBaseUI().getWidth();
      final float curR = currentLeft + getWidth();
      if (curR + mSticky.right > parentWidth) {
        mSticky.x = Math.max(parentWidth - curR - mSticky.right, mSticky.left - currentLeft);
      } else {
        mSticky.x = 0;
      }
    }

    if (currentTop < mSticky.top) {
      mSticky.y = mSticky.top - currentTop;
    } else {
      if (getParentBaseUI() == null) {
        LLog.e(TAG, "checkStickyOnParentScroll failed, parent is null.");
        return false;
      }

      final int parentHeight = getParentBaseUI().getHeight();
      final float currentBottom = currentTop + getHeight();
      if (currentBottom + mSticky.bottom > parentHeight) {
        mSticky.y =
            Math.max(parentHeight - currentBottom - mSticky.bottom, mSticky.top - currentTop);
      } else {
        mSticky.y = 0;
      }
    }

    return true;
  }

  /********** EvnetTarget Section **********/

  @Override
  public int getSign() {
    return mSign;
  }

  // for gesture arena
  @Override
  public int getGestureArenaMemberId() {
    return mGestureArenaMemberId;
  }

  @Override
  public int getPseudoStatus() {
    return mPseudoStatus;
  }

  @Override
  public EventTarget parent() {
    if (mParent instanceof EventTarget) {
      return (EventTarget) mParent;
    }
    return null;
  }

  @Override
  public EventTargetBase parentResponder() {
    if (mParent instanceof EventTargetBase) {
      return (EventTargetBase) mParent;
    }
    return null;
  }

  public float[] getTargetPoint(
      float x, float y, int scrollX, int scrollY, Rect targetRect, Matrix transformMatrix) {
    float[] targetPoint = new float[2];
    targetPoint[0] = x + scrollX - targetRect.left;
    targetPoint[1] = y + scrollY - targetRect.top;

    Matrix inverseMatrix = getHitTestMatrix();

    // When the front-end developer sets transform:scale(0), transform:scaleX(0), or
    // transform:scaleY(0), the transform matrix of the Android View will fail to generate an
    // inverse matrix. At the same time, when the above properties are set, the current UI will
    // definitely not contain the point, so let targetPoint[0]=Float.MAX_VALUE,
    // targetPoint[1]=Float.MAX_VALUE directly.
    if (transformMatrix.invert(inverseMatrix)) {
      float[] tempPoint = {targetPoint[0], targetPoint[1]};
      inverseMatrix.mapPoints(tempPoint);
      targetPoint[0] = tempPoint[0];
      targetPoint[1] = tempPoint[1];
    } else {
      targetPoint[0] = Float.MAX_VALUE;
      targetPoint[1] = Float.MAX_VALUE;
    }
    return targetPoint;
  }

  public float[] getTargetPoint(
      float x, float y, int scrollX, int scrollY, View targetView, Matrix transformMatrix) {
    return getTargetPoint(x, y, scrollX, scrollY,
        new Rect(targetView.getLeft(), targetView.getTop(), 0, 0), transformMatrix);
  }

  public float[] getLocationOnScreen(final float[] point) {
    int[] base_point = new int[2];

    if (isFlatten()) {
      point[0] += getLeft();
      point[1] += getTop();
    }

    View view = null;
    if (isFlatten()) {
      if (mDrawParent == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value getLocationOnScreen returns is not the correct coordinates relative to the screen!");
        return point;
      }
      view = ((LynxUI) mDrawParent).getView();
      point[0] -= view.getScrollX();
      point[1] -= view.getScrollY();
    } else {
      view = ((LynxUI) this).getView();
    }

    // get the position of the view relative to the screen
    // point: currentView to rootView
    // base_point: rootView to screen
    // currentView to Screen: point + basepoint
    View rootView = view.getRootView();
    rootView.getLocationOnScreen(base_point);
    transformFromViewToRootView(view, point);

    point[0] += base_point[0];
    point[1] += base_point[1];
    return point;
  }

  public boolean isCustomHittest() {
    return false;
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  // use ignoreUserInteraction flag to distinguish the usage of hitTest between hit response chain
  // and devtool getNodeForLocation when call hitTest for getNodeForLocation, if ui.getVisibility()
  // returns true, then node can be added to response chain when call hitTest for hit response
  // chain, If set user-interaction-enabled="{{false}}" or visibility: hidden, this ui will not be
  // on the response chain.
  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    LynxBaseUI target = null;

    float child_x = x;
    float child_y = y;
    for (int i = mChildren.size() - 1; i >= 0; i--) {
      LynxBaseUI ui = mChildren.get(i);
      if (ui instanceof UIShadowProxy) {
        ui = ((UIShadowProxy) ui).getChild();
      }

      // when ignoreUserInteraction set false:
      // only nodes that are visible and userInteraction true will be added to the response chain.
      //
      // when ignoreUserInteraction set true:
      // nodes only need to be visible will be added to the reponse chain
      // TODO(hexionghui): use shouldHitTest interface.
      if ((!ignoreUserInteraction && !ui.isUserInteractionEnabled()) || !ui.getVisibility()
          || (ui.getDrawParent() != null && ((LynxUI) ui.getDrawParent()).getView() != null
              && ((LynxUI) ui.getDrawParent()).getView().getParent() == null)) {
        continue;
      }

      boolean contain = false;
      float[] point = new float[] {x, y};
      if (mContext.getEnableEventRefactor()) {
        // If EnableEventRefactor, transform point from ancestor to descendants, consider the
        // transform props.
        point = getTargetPoint(point[0], point[1], getScrollX(), getScrollY(),
            ui.getRectWithoutTransform(), ui.getTransformMatrix());
        contain = ui.containsPoint(point[0], point[1], ignoreUserInteraction);
      } else {
        contain = ui.containsPoint(point[0], point[1], ignoreUserInteraction);
      }

      if (contain) {
        if (ui.isOnResponseChain()) {
          target = ui;
          child_x = point[0];
          child_y = point[1];
          break;
        }
        if (target == null || target.getRealTimeTranslationZ() < ui.getRealTimeTranslationZ()) {
          target = ui;
          child_x = point[0];
          child_y = point[1];
        }
      }
    }

    if (target == null) {
      return this;
    }

    if (!target.isCustomHittest() && target.needCustomLayout() && target instanceof UIGroup) {
      if (mContext.getEnableEventRefactor()) {
        return ((UIGroup) target).findUIWithCustomLayout(child_x, child_y, (UIGroup) target);
      }
      return ((UIGroup) target)
          .findUIWithCustomLayout(
              x - target.getOriginLeft(), y - target.getOriginTop(), (UIGroup) target);
    }
    if (mContext.getEnableEventRefactor()) {
      x = child_x;
      y = child_y;
      return target.hitTest(x, y, ignoreUserInteraction);
    }

    x = x + target.getScrollX() - target.getOriginLeft() - target.getTranslationX();
    y = y + target.getScrollY() - target.getOriginTop() - target.getTranslationY();
    return target.hitTest(x, y, ignoreUserInteraction);
  }

  @Override
  public boolean containsPoint(float x, float y) {
    return containsPoint(x, y, false);
  }

  @Override
  public boolean containsPoint(float x, float y, boolean ignoreUserInteraction) {
    float slop = getTouchSlop();
    boolean contain = false;
    if (mContext.getEnableEventRefactor()) {
      // If EnableEventRefactor, the point is converted according to the child's origin.
      Rect rect = getRectWithoutTransform();
      contain = -slop - mHitSlopLeft <= x && rect.right - rect.left + slop + mHitSlopRight >= x
          && -slop - mHitSlopTop <= y && rect.bottom - rect.top + slop + mHitSlopBottom >= y;
      if (!contain && getOverflow() != 0) {
        if (getOverflow() == OVERFLOW_X) {
          if (y < rect.top || y > rect.bottom) {
            return contain;
          }
        } else if (getOverflow() == OVERFLOW_Y) {
          if (x < rect.left || x > rect.right) {
            return contain;
          }
        }
        contain = childrenContainPoint(x, y, ignoreUserInteraction);
      }
      return contain;
    }

    Rect rect = getRect();
    contain = rect.left - slop < x && rect.right + slop > x && rect.top - slop < y
        && rect.bottom + slop > y;
    // currently, do not care about flatten ui translation
    if (!contain && getOverflow() != 0) {
      if (getOverflow() == OVERFLOW_X) {
        if (!(rect.top - slop < y && rect.bottom + slop > y)) {
          return contain;
        }
      } else if (getOverflow() == OVERFLOW_Y) {
        if (!(rect.left - slop < x && rect.right + slop > x)) {
          return contain;
        }
      }
      contain = childrenContainPoint(x, y, ignoreUserInteraction);
    }
    return contain;
  }

  public boolean childrenContainPoint(float x, float y) {
    return childrenContainPoint(x, y, false);
  }

  public boolean childrenContainPoint(float x, float y, boolean ignoreUserInteraction) {
    if (mContext.getEnableEventRefactor()) {
      for (LynxBaseUI child : mChildren) {
        if (child instanceof UIShadowProxy) {
          child = ((UIShadowProxy) child).getChild();
        }
        float[] targetPoint = getTargetPoint(x, y, getScrollX(), getScrollY(),
            child.getRectWithoutTransform(), child.getTransformMatrix());

        // when ignoreUserInteraction set false:
        // only nodes that are visible and userInteraction true will be added to the response chain.
        //
        // when ignoreUserInteraction set true:
        // nodes only need to be visible will be added to the reponse chain
        if ((ignoreUserInteraction || child.isUserInteractionEnabled()) && child.getVisibility()
            && child.containsPoint(targetPoint[0], targetPoint[1], ignoreUserInteraction)) {
          return true;
        }
      }
      return false;
    }

    x = x + getScrollX() - getOriginLeft() - getTranslationX();
    y = y + getScrollY() - getOriginTop() - getTranslationY();
    for (LynxBaseUI child : mChildren) {
      // when ignoreUserInteraction set false:
      // only nodes that are visible and userInteraction true will be added to the response chain.
      //
      // when ignoreUserInteraction set true:
      // nodes only need to be visible will be added to the reponse chain
      if ((ignoreUserInteraction || child.isUserInteractionEnabled()) && child.getVisibility()
          && child.containsPoint(x, y, ignoreUserInteraction)) {
        return true;
      }
    }
    return false;
  }

  @Override
  public Map<String, EventsListener> getEvents() {
    return mEvents;
  }

  // key is gesture id, value is gesture detector
  public Map<Integer, GestureDetector> getGestureDetectorMap() {
    return mGestureDetectors;
  }

  @Override
  public Matrix getTransformMatrix() {
    mTransformMatrix.reset();
    return mTransformMatrix;
  }

  @Override
  public boolean isUserInteractionEnabled() {
    return this.userInteractionEnabled;
  }

  @Override
  public boolean ignoreFocus() {
    // If mIgnoreFocus == Enable, return true. If mIgnoreFocus == Disable, return false.
    // If mIgnoreFocus == Undefined && parent not null, return parent.ignoreFocus()
    if (mIgnoreFocus == EnableStatus.Enable) {
      return true;
    } else if (mIgnoreFocus == EnableStatus.Disable) {
      return false;
    } else if (parent() != null) {
      EventTarget parent = parent();
      // when parent is root ui, return false.
      if (parent instanceof UIBody) {
        return false;
      }
      return parent.ignoreFocus();
    }
    return false;
  }

  @Override
  public boolean isFocusable() {
    return mFocusable;
  }

  @Override
  public boolean isScrollable() {
    return false;
  }

  public boolean isOverlay() {
    return false;
  }

  @Override
  public boolean isClickable() {
    return mEvents != null && mEvents.containsKey("tap");
  }

  @Override
  public boolean isLongClickable() {
    return mEvents != null && mEvents.containsKey("longpress");
  }

  @Override
  public boolean enableTouchPseudoPropagation() {
    return mEnableTouchPseudoPropagation;
  }

  @Override
  public void onPseudoStatusChanged(int preStatus, int currentStatus) {
    mPseudoStatus = currentStatus;
  }

  @Override
  public void onFocusChanged(boolean hasFocus, boolean isFocusTransition) {}

  @Override
  public void onResponseChain() {
    mOnResponseChain = true;
  }

  @Override
  public void offResponseChain() {
    mOnResponseChain = false;
  }

  @Override
  public boolean isOnResponseChain() {
    return mOnResponseChain;
  }

  @Override
  public boolean consumeSlideEvent(float angle) {
    // If mConsumeSlideEventAngles is null, return false indicating that the current LynxUI does not
    // consume slide events. Otherwise, traverse mConsumeSlideEventAngles and check if the given
    // angle falls within each angle interval. If the condition is met, return true, indicating that
    // the current LynxUI needs to consume slide events.
    if (mConsumeSlideEventAngles == null) {
      return false;
    }
    for (ArrayList<Float> ary : mConsumeSlideEventAngles) {
      if (angle >= ary.get(0) && angle <= ary.get(1)) {
        return true;
      }
    }
    return false;
  }

  @Override
  public boolean blockNativeEvent(MotionEvent ev) {
    boolean blockNativeEventAll = this.mBlockNativeEvent;
    if (blockNativeEventAll) {
      return true;
    }
    if (this.mBlockNativeEventAreas == null) {
      return false;
    }

    boolean blockNativeEventThisPoint = false;
    Rect clientRect = getLynxContext().getUIBody().getBoundingClientRect();
    Rect viewRect = getBoundingClientRect();
    LynxTouchEvent.Point clientPoint = new LynxTouchEvent.Point(ev.getX(), ev.getY());
    LynxTouchEvent.Point viewPoint = clientPoint.convert(clientRect, viewRect);

    for (int i = 0; i < this.mBlockNativeEventAreas.size(); ++i) {
      ArrayList<SizeValue> area = this.mBlockNativeEventAreas.get(i);
      if (area != null && area.size() == 4) {
        float left = area.get(0).convertToDevicePx(viewRect.right - viewRect.left);
        float top = area.get(1).convertToDevicePx(viewRect.bottom - viewRect.top);
        float right = left + area.get(2).convertToDevicePx(viewRect.right - viewRect.left);
        float bottom = top + area.get(3).convertToDevicePx(viewRect.bottom - viewRect.top);
        blockNativeEventThisPoint = viewPoint.getX() >= left && viewPoint.getX() < right
            && viewPoint.getY() >= top && viewPoint.getY() < bottom;
        if (blockNativeEventThisPoint) {
          LLog.i(TAG, "blocked this point!");
          break;
        }
      }
    }
    return blockNativeEventThisPoint;
  }

  @Override
  public boolean eventThrough() {
    // If mEventThrough == Enable, return true. If mEventThrough == Disable, return false.
    // If mEventThrough == Undefined && parent not null, return parent.eventThrough()
    if (mEventThrough == EnableStatus.Enable) {
      return true;
    } else if (mEventThrough == EnableStatus.Disable) {
      return false;
    } else if (parent() != null) {
      EventTarget parent = parent();
      // when parent is root ui, return false.
      if (parent instanceof UIBody) {
        return false;
      }
      return parent.eventThrough();
    }
    return false;
  }

  // default return false. Return true if this LynxUI is scroll container, like scroll-view, swiper,
  // list and so on.
  public boolean isScrollContainer() {
    return false;
  }

  // if return true, frontend event will not be sent
  public boolean dispatchEvent(LynxEventDetail event) {
    return false;
  }

  @Override
  public boolean dispatchTouch(MotionEvent ev) {
    return false;
  }

  private float getTouchSlop() {
    if (mOnResponseChain) {
      return mTouchSlop * mContext.getResources().getDisplayMetrics().density;
    }
    return 0;
  }

  /******** EvnetTarget Section End **********/

  public void registerScrollStateListener(ScrollStateChangeListener listener) {
    if (listener == null) {
      return;
    }
    synchronized (this) {
      if (mStateChangeListeners == null) {
        mStateChangeListeners = new HashSet<>();
      }
      mStateChangeListeners.add(listener);
      if (mStateChangeListeners.size() != 1) {
        return;
      }
    }
    initScrollStateChangeListener();
    UIParent parent = getParent();
    if (parent instanceof LynxBaseUI) {
      ((LynxBaseUI) parent).registerScrollStateListener(mScrollStateChangeListener);
    }
  }

  private synchronized void initScrollStateChangeListener() {
    if (mScrollStateChangeListener != null) {
      return;
    }
    mScrollStateChangeListener = new ScrollStateChangeListener() {
      @Override
      public void onScrollStateChanged(int state) {
        ScrollStateChangeListener[] listeners;
        synchronized (LynxBaseUI.this) {
          listeners = mStateChangeListeners.toArray(
              new ScrollStateChangeListener[mStateChangeListeners.size()]);
        }
        for (ScrollStateChangeListener changeListener : listeners) {
          changeListener.onScrollStateChanged(state);
        }
      }
    };
  }

  public void unRegisterScrollStateListener(ScrollStateChangeListener listener) {
    if (listener == null || mStateChangeListeners == null) {
      return;
    }
    boolean empty;
    synchronized (this) {
      mStateChangeListeners.remove(listener);
      empty = mStateChangeListeners.isEmpty();
    }
    if (empty) {
      UIParent parent = getParent();
      if (parent instanceof LynxBaseUI) {
        ((LynxBaseUI) parent).unRegisterScrollStateListener(mScrollStateChangeListener);
      }
    }
  }

  public void notifyScrollStateChanged(int state) {
    if (mStateChangeListeners == null) {
      return;
    }
    ScrollStateChangeListener[] listeners;
    synchronized (this) {
      listeners = mStateChangeListeners.toArray(
          new ScrollStateChangeListener[mStateChangeListeners.size()]);
    }
    for (ScrollStateChangeListener listener : listeners) {
      listener.onScrollStateChanged(state);
    }
  }

  /**
   * The bytecode of this method will be modified during compilation.
   * Do not rewrite this method or modify this method.
   *
   * @param map
   */
  @Override
  public void dispatchProperties(StylesDiffMap map) {}

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public Point getLastSize() {
    return mLastSize;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public Point getLatestSize() {
    return mLatestSize;
  }

  /**
   * Copy animated related props from another UI.
   * Should only be called in LynxUIOwner.updateFlatten
   *
   * @param oldUI
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void copyPropFromOldUiInUpdateFlatten(LynxBaseUI oldUI) {
    mIsFirstAnimatedReady = oldUI.mIsFirstAnimatedReady;
    mLastSize.set(oldUI.getLastSize().x, oldUI.getLastSize().y);
    mLatestSize.set(oldUI.getLatestSize().x, oldUI.getLatestSize().y);
    // For translateZ.
    mFlattenChildrenCount = oldUI.mFlattenChildrenCount;
    setEvents(oldUI.getEvents());
  }

  public boolean getNeedSortChildren() {
    return mNeedSortChildren;
  }

  public void setNeedSortChildren(boolean needSortChildren) {
    mNeedSortChildren = needSortChildren;
  }

  public float getLastTranslateZ() {
    return mLastTranslateZ;
  }

  public void setLastTranslateZ(float lastTranslateZ) {
    mLastTranslateZ = lastTranslateZ;
  }

  public void setOffsetDescendantRectToLynxView(int[] offset) {
    mOffsetDescendantRectToLynxView = new WeakReference<int[]>(offset);
  }

  public int[] getOffsetDescendantRectToLynxView() {
    int[] offset = mOffsetDescendantRectToLynxView.get();
    if (offset != null) {
      return offset;
    } else {
      return sDefaultOffsetToLynxView;
    }
  }

  // Warning: just use for hittest, do not use this in other place.
  public Matrix getHitTestMatrix() {
    mHitTestMatrix.reset();
    return mHitTestMatrix;
  }

  /**
   * if the listCell is appear,the callBack will be invoked
   *
   * @param itemKey the itemKey of listCell
   * @param list    list
   */
  public void onListCellAppear(String itemKey, LynxBaseUI list) {
    if (mBlockListEvent) {
      return;
    }
    for (LynxBaseUI child : mChildren) {
      child.onListCellAppear(itemKey, list);
    }
  }

  /**
   * the listCell is disAppear,the callBack wibll be invoked
   *
   * @param itemKey the itemKey of listCell
   * @param list    list
   * @param isExist the listcell is still exist
   */
  public void onListCellDisAppear(String itemKey, LynxBaseUI list, boolean isExist) {
    if (mBlockListEvent) {
      return;
    }
    for (LynxBaseUI child : mChildren) {
      child.onListCellDisAppear(itemKey, list, isExist);
    }
  }

  /**
   * the listCell is resued,the callBack wibll be invoked
   *
   * @param itemKey the itemKey of listCell
   * @param list    list
   */
  public void onListCellPrepareForReuse(String itemKey, LynxBaseUI list) {
    if (mBlockListEvent) {
      return;
    }
    for (LynxBaseUI child : mChildren) {
      child.onListCellPrepareForReuse(itemKey, list);
    }
  }

  public String constructListStateCacheKey(String tagName, String itemKey, String idSelector) {
    return tagName + "_" + itemKey + "_" + (idSelector != null ? idSelector : "");
  }

  public void removeKeyFromNativeStorage(String key) {}

  public void storeKeyToNativeStorage(String key, Object value) {}

  public Object getValueFromNativeStorage(String key) {
    return null;
  }

  public boolean initialPropsFlushed(String initialPropKey, String cacheKey) {
    return true;
  }

  public void setInitialPropsHasFlushed(String initialPropKey, String cacheKey) {}

  @NonNull
  @Override
  public LynxBaseUI clone() throws CloneNotSupportedException {
    LynxBaseUI ui = mContext.getLynxUIOwner().createUI(mTagName, false);

    applyUIPaintStylesToTarget(ui);

    for (LynxBaseUI child : getChildren()) {
      LynxBaseUI cpChild = child.clone();
      int index = ui.getChildren().size();
      ui.insertChild(cpChild, index);
      ((UIGroup) ui).insertView((LynxUI) cpChild);
    }
    ui.updateProperties(new StylesDiffMap(mProps));
    ui.updateLayoutSize(getWidth(), getHeight());
    ui.updateLayout(getLeft(), getTop(), getWidth(), getHeight(), getPaddingLeft(), getPaddingTop(),
        getPaddingRight(), getPaddingBottom(), getMarginLeft(), getMarginTop(), getMarginRight(),
        ui.getMarginBottom(), getBorderLeftWidth(), getBorderTopWidth(), getBorderRightWidth(),
        getBorderBottomWidth(), getBound());

    ui.onLayoutFinish(0, null);

    ui.onNodeReady();

    return ui;
  }

  @Nullable
  public GestureArenaManager getGestureArenaManager() {
    if (mContext == null) {
      return null;
    }
    LynxUIOwner uiOwner = mContext.getLynxUIOwner();
    if (uiOwner == null) {
      return null;
    }
    // Return the GestureArenaManager from the uiOwner
    return uiOwner.getGestureArenaManager();
  }

  @Override
  public void setGestureDetectorState(int gestureId, int state) {
    if (!isEnableNewGesture()) {
      return;
    }
    GestureArenaManager manager = getGestureArenaManager();
    if (manager == null) {
      return;
    }
    manager.setGestureDetectorState(getGestureArenaMemberId(), gestureId, state);
  }

  @Override
  public void consumeGesture(int gestureId, ReadableMap params) {
    if (!isEnableNewGesture() || params == null) {
      return;
    }
    boolean inner = params.getBoolean("inner", true);
    boolean consume = params.getBoolean("consume", true);
    if (inner) {
      consumeGesture(consume);
    }
  }

  protected void consumeGesture(boolean consumeGesture) {
    // Implemented in child
  }

  public boolean isEnableNewGesture() {
    return mGestureArenaMemberId > 0;
  }

  /**
   * @name: hit-slop
   * @description: Change the margin of ui for hit-test. A positive value means to expand the
   * boundary, and a negative value means to shrink the boundary. The changes in the four directions
   * can be controlled by top, bottom, left, and right. When no direction is specified, it means
   * that the four directions are changed at the same time.
   * @category: stable
   * @standardAction: keep
   * @supportVersion: 2.14
   **/
  @LynxProp(name = PropsConstants.HIT_SLOP)
  public void setHitSlop(@Nullable Dynamic value) {
    if (value == null
        || !(value.getType() == ReadableType.Map || value.getType() == ReadableType.String)) {
      return;
    }

    if (value.getType() == ReadableType.Map && value.asMap().size() > 0) {
      ReadableMap dict = value.asMap();
      ReadableMapKeySetIterator it = dict.keySetIterator();
      List keySet = Arrays.asList(PropsConstants.HIT_SLOP_TOP, PropsConstants.HIT_SLOP_BOTTOM,
          PropsConstants.HIT_SLOP_LEFT, PropsConstants.HIT_SLOP_RIGHT);
      while (it.hasNextKey()) {
        String key = it.nextKey();
        if (keySet.contains(key)) {
          float ptValue = UnitUtils.toPx(dict.getString(key));
          switch (keySet.indexOf(key)) {
            case 0:
              mHitSlopTop = mHitSlopTop != ptValue ? ptValue : mHitSlopTop;
              break;
            case 1:
              mHitSlopBottom = mHitSlopBottom != ptValue ? ptValue : mHitSlopBottom;
              break;
            case 2:
              mHitSlopLeft = mHitSlopLeft != ptValue ? ptValue : mHitSlopLeft;
              break;
            case 3:
              mHitSlopRight = mHitSlopRight != ptValue ? ptValue : mHitSlopRight;
              break;
            default:
              break;
          }
        }
      }
      return;
    }

    float ptValue = UnitUtils.toPx(value.asString());
    if (mHitSlopTop != ptValue || mHitSlopBottom != ptValue || mHitSlopLeft != ptValue
        || mHitSlopRight != ptValue) {
      mHitSlopTop = mHitSlopBottom = mHitSlopLeft = mHitSlopRight = ptValue;
    }
  }

  public Window getWindow() {
    return ContextUtils.getWindow(mContext);
  }

  public TouchEventDispatcher getTouchEventDispatcher() {
    if (mContext != null) {
      return mContext.getTouchEventDispatcher();
    } else {
      return null;
    }
  }
}
