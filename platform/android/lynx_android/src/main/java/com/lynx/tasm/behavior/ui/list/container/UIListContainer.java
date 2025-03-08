// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import static com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView.LIST_AUTOMATIC_MAX_FLING_RATIO;
import static com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView.SCROLL_STATE_DRAGGING;
import static com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView.SCROLL_STATE_IDLE;
import static com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView.SCROLL_STATE_SCROLL_ANIMATION;

import android.content.Context;
import android.os.Build;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.widget.LinearLayout;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.ListNodeInfoFetcher;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.list.ListEventManager;
import com.lynx.tasm.behavior.ui.list.LynxSnapHelper;
import com.lynx.tasm.behavior.ui.view.ComponentView;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.ui.view.UISimpleView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.event.LynxScrollEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;

public class UIListContainer extends UISimpleView<ListContainerView>
    implements NestedScrollContainerView.OnScrollStateChangeListener, GestureArenaMember {
  private static final String TAG = "UIListContainer";
  public static final int INVALID_SCROLL_ESTIMATED_OFFSET = -1;
  private static final boolean DEBUG = false;
  private static final int DEFAULT_FADE_IN_ANIMATION_DURATION = 100;
  private boolean mIsVertical = true;
  private UIListAutoScroller mAutoScroller = null;
  private JavaOnlyArray mItemKeys = new JavaOnlyArray();
  private final HashMap<String, Integer> mItemKeyMap = new HashMap<>();
  private boolean mEnableListSticky = false;
  private int mStickyOffset = 0;
  public Map<String, Object> nativeListStateCache = new HashMap<>();
  public Map<String, HashSet<String>> initialFlushPropCache = new HashMap<>();
  private final HashMap<Integer, UIComponent> mStickyTopItems = new LinkedHashMap<>();
  private final HashMap<Integer, UIComponent> mStickyBottomItems = new LinkedHashMap<>();
  private JavaOnlyArray mStickyTopIndexes = new JavaOnlyArray();
  private JavaOnlyArray mStickyBottomIndexes = new JavaOnlyArray();
  private UIComponent mPrevStickyTopItem = null;
  private UIComponent mPrevStickyBottomItem = null;
  private boolean mEnableFadeInAnimation = false;
  private int mUpdateAnimationFadeInDuration = DEFAULT_FADE_IN_ANIMATION_DURATION;
  private boolean mEnableScrollEndEvent = false;
  private boolean mEnableScrollStateChangeEvent = false;
  private boolean mEnableBatchRender = false;
  private boolean mEnableNeedVisibleItemInfo = false;
  // for new gesture
  private Map<Integer, BaseGestureHandler> mGestureHandlers;

  private Callback mScrollToCallback = null;
  private int mScrollingEstimatedOffset = INVALID_SCROLL_ESTIMATED_OFFSET;

  private final NestedScrollContainerView.CustomScrollHook mCustomScrollHook =
      new NestedScrollContainerView.CustomScrollHook() {
        private int mInitialScrollingEstimatedOffset = INVALID_SCROLL_ESTIMATED_OFFSET;
        private boolean mScrollToLower = false;

        @Override
        public void onSmoothScrollStart(
            int lastScrollX, int lastScrollY, int scrollX, int scrollY) {
          // Try to scroll to the estimated offset
          mInitialScrollingEstimatedOffset = !mIsVertical ? scrollX : scrollY;
          mScrollToLower =
              mInitialScrollingEstimatedOffset > (mIsVertical ? lastScrollY : lastScrollX);
        }

        @Override
        public void onSmoothScrollEnd() {
          LynxContext context = getLynxContext();
          if (context != null && context.getListNodeInfoFetcher() != null) {
            getLynxContext().getListNodeInfoFetcher().scrollStopped(getSign());
          }
        }

        @Override
        public void onSmoothScroll(int scrollX, int scrollY, @NonNull int[] targetScrollOffset) {
          // Ensure that our offset does not exceed the estimated value. As the estimate changes
          // constantly, adjust the distance of each scroll linearly.

          int offset = !mIsVertical ? scrollX : scrollY;
          if (mInitialScrollingEstimatedOffset != 0) {
            offset = (int) (offset
                * (mScrollingEstimatedOffset * 1.f / mInitialScrollingEstimatedOffset));
          }
          if (mScrollingEstimatedOffset > 0) {
            if ((mScrollToLower && offset > mScrollingEstimatedOffset)
                || (!mScrollToLower && offset < mScrollingEstimatedOffset)) {
              offset = mScrollingEstimatedOffset;
            }
          }
          targetScrollOffset[0] = mIsVertical ? scrollX : offset;
          targetScrollOffset[1] = mIsVertical ? offset : scrollY;
        }
      };

  public UIListContainer(LynxContext context) {
    super(context);
  }

  @Override
  protected ListContainerView createView(Context context) {
    LLog.i(TAG, "create UIListContainer");
    final ListContainerView listContainerView = new ListContainerView(context, this);
    listContainerView.setOnScrollStateChangeListener(this);
    return listContainerView;
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    // The list container should manager the view of component on the interface of
    // "onLayoutFinish()"
    super.onInsertChild(child, index);

    if (mEnableListSticky) {
      int indexFromItemKey = getIndexFromItemKey(((UIComponent) child).getItemKey());
      updateStickyInfoForInsertedChild(child, mStickyTopItems, mStickyTopIndexes, indexFromItemKey);
      updateStickyInfoForInsertedChild(
          child, mStickyBottomItems, mStickyBottomIndexes, indexFromItemKey);
    }
  }

  @Override
  public void removeChild(LynxBaseUI child) {
    super.removeChild(child);
    if (child instanceof UIComponent) {
      UIComponent component = (UIComponent) child;
      if (mEnableFadeInAnimation && isAsyncThreadStrategy()) {
        View childView = component.getView();
        if (childView != null && childView.getAnimation() != null) {
          childView.getAnimation().cancel();
        }
      }
      component.setOnUpdateListener(null);
    }
    updateStickyInfoForDeletedChild(child, mStickyTopItems);
    updateStickyInfoForDeletedChild(child, mStickyBottomItems);
  }

  @Override
  public void onLayoutFinish(long operationId, @Nullable LynxBaseUI component) {
    super.onLayoutFinish(operationId, component);
    if (!mEnableBatchRender) {
      insertListItemNode(component);
    }
  }

  public void insertListItemNode(@Nullable LynxBaseUI component) {
    if (component instanceof UIComponent) {
      UIComponent child = (UIComponent) component;
      if (mEnableBatchRender) {
        insertListItemNodeInternal(child);
      } else {
        // In multi thread, the child component has been rendered when invoking onLayoutFinish() but
        // may not have any layout info, because we block the child component layout info's flushing
        // which triggered by starlight engine, so we cannot add child view to the list until
        // getting the real layout info of the child component.
        if (child.getWidth() != 0 || child.getHeight() != 0) {
          insertListItemNodeInternal(child);
        } else if (child.getOnUpdateListener() == null) {
          child.setOnUpdateListener(new UIComponent.OnUpdateListener() {
            @Override
            public void onLayoutUpdated(UIComponent ui) {
              // Get layout info, and add the child view to parent.
              if (ui != null) {
                insertListItemNodeInternal(ui);
              }
            }
          });
        }
      }
      if (mEnableListSticky) {
        int indexFromItemKey = getIndexFromItemKey(child.getItemKey());
        updateStickyInfoForUpdatedChild(
            child, mStickyTopItems, mStickyTopIndexes, indexFromItemKey);
        updateStickyInfoForUpdatedChild(
            child, mStickyBottomItems, mStickyBottomIndexes, indexFromItemKey);
      }
    }
  }

  private void insertListItemNodeInternal(@Nullable UIComponent component) {
    View childView = component.getView();
    if (childView != null && childView.getParent() == null) {
      mView.addView(childView);
      startFadeInAnimation(childView);
    }
    setChildTranslationZ(component);
  }

  // Here we use TranslationZ to implement z-index, this is because subviews can be reused in list,
  // and if we change the order of subviews in the mChildren array to implement z-index, the
  // additional removal and insertion will be performed on some subview, which leads to the problem
  // of duplicate layout and it also affects the internal logic of the subview.
  private void setChildTranslationZ(UIComponent child) {
    if (child != null) {
      setChildTranslationZ(child, child.getZIndex());
    }
  }

  private void setChildTranslationZ(UIComponent child, float translateZ) {
    View childView = child.getView();
    if (childView != null) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        childView.setOutlineProvider(null);
        ViewCompat.setTranslationZ(childView, translateZ);
      }
    }
  }

  public boolean isAsyncThreadStrategy() {
    if (mContext == null || mContext.getLynxView() == null) {
      return false;
    }
    ThreadStrategyForRendering strategy = mContext.getLynxView().getThreadStrategyForRendering();
    return strategy == ThreadStrategyForRendering.MOST_ON_TASM
        || strategy == ThreadStrategyForRendering.MULTI_THREADS;
  }

  private void startFadeInAnimation(View childView) {
    if (mEnableFadeInAnimation && childView != null && isAsyncThreadStrategy()) {
      AlphaAnimation fadeInAnimation = new AlphaAnimation(0, 1);
      fadeInAnimation.setDuration(mUpdateAnimationFadeInDuration);
      childView.startAnimation(fadeInAnimation);
    }
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    updateStickyTops(mView.getScrollY());
    updateStickyBottoms(mView.getScrollY());
  }

  @Override
  public void removeKeyFromNativeStorage(String key) {
    if (nativeListStateCache != null) {
      nativeListStateCache.remove(key);
    }
  }

  @Override
  public void storeKeyToNativeStorage(String key, Object value) {
    if (nativeListStateCache != null) {
      nativeListStateCache.put(key, value);
    }
  }

  @Override
  public boolean initialPropsFlushed(String initialPropKey, String cacheKey) {
    if (initialFlushPropCache != null) {
      HashSet<String> initialPropFlushSet = initialFlushPropCache.get(cacheKey);
      return initialPropFlushSet != null && initialPropFlushSet.contains(initialPropKey);
    }
    return false;
  }

  @Override
  public void setInitialPropsHasFlushed(String initialPropKey, String cacheKey) {
    if (initialFlushPropCache != null) {
      HashSet<String> initialPropFlushSet = initialFlushPropCache.get(cacheKey) == null
          ? new HashSet<String>()
          : initialFlushPropCache.get(cacheKey);
      initialPropFlushSet.add(initialPropKey);
      initialFlushPropCache.put(cacheKey, initialPropFlushSet);
    }
  }

  public Object getValueFromNativeStorage(String key) {
    if (nativeListStateCache != null) {
      return nativeListStateCache.get(key);
    }
    return null;
  }

  @Override
  public void invalidate() {
    if (mView.getLinearLayout() != null) {
      mView.getLinearLayout().invalidate();
    }
    super.invalidate();
  }

  @Override
  public void destroy() {
    super.destroy();
    if (mView != null) {
      mView.destroy();
    }
    if (mAutoScroller != null) {
      mAutoScroller.removeFrameCallback();
    }
    // remove arena member if destroy
    GestureArenaManager manager = getGestureArenaManager();
    if (manager != null) {
      manager.removeMember(this);
    }
    // clear gesture map if destroy
    if (mGestureHandlers != null) {
      mGestureHandlers.clear();
    }
  }

  public void updateContentSizeAndOffset(float contentSize, float deltaX, float deltaY) {
    if (DEBUG) {
      LLog.i(TAG, "updateContentSizeAndOffset: " + contentSize + ", " + deltaX + ", " + deltaY);
    }
    mView.updateContentSizeAndOffset((int) contentSize, (int) deltaX, (int) deltaY);
  }

  @LynxProp(name = "item-snap")
  public void setPagingAlignment(ReadableMap map) {
    if (map instanceof JavaOnlyMap) {
      JavaOnlyMap params = (JavaOnlyMap) map;
      if (params.size() != 0) {
        double factor = map.getDouble("factor");
        if (factor < 0 || factor > 1) {
          getLynxContext().handleLynxError(new LynxError(
              LynxSubErrorCode.E_COMPONENT_LIST_INVALID_PROPS_ARG, "item-snap invalid!",
              "The factor should be constrained to the range of [0,1].", LynxError.LEVEL_WARN));
          factor = 0;
        }
        int offset = map.getInt("offset", 0);

        // The logic here is copied form the SnapHelper provided by Android.
        double snapAlignmentMillisecondsPerPx = 100f / mContext.getScreenMetrics().densityDpi;

        mView.mSnapHelper = new LynxSnapHelper(
            factor, offset, snapAlignmentMillisecondsPerPx, new LynxSnapHelper.LynxSnapHooks() {
              @Override
              public int getScrollX() {
                return getView().getScrollX();
              }

              @Override
              public int getScrollY() {
                return getView().getScrollY();
              }

              @Override
              public int getScrollHeight() {
                return getView().getHeight();
              }

              @Override
              public int getScrollWidth() {
                return getView().getWidth();
              }

              @Override
              public int getChildrenCount() {
                return getView().getLinearLayout().getChildCount();
              }

              @Override
              public int getVirtualChildrenCount() {
                return mItemKeys.size();
              }

              @Override
              public View getChildAtIndex(int index) {
                return getView().getLinearLayout().getChildAt(index);
              }

              @Override
              public View getViewAtPosition(int position) {
                String itemKey = (String) mItemKeys.get(position);
                for (int i = 0; i < mView.getLinearLayout().getChildCount(); i++) {
                  View view = mView.getLinearLayout().getChildAt(i);
                  if (itemKey.equals(
                          ((UIComponent) ((ComponentView) view).getDrawChildHook()).getItemKey())) {
                    return view;
                  }
                }
                return null;
              }

              @Override
              public int getIndexFromView(View view) {
                if (view instanceof ComponentView
                    && ((ComponentView) view).getDrawChildHook() instanceof UIComponent) {
                  String itemKey =
                      ((UIComponent) ((ComponentView) view).getDrawChildHook()).getItemKey();
                  if (mItemKeys.contains(itemKey)) {
                    return mItemKeys.indexOf(itemKey);
                  }
                }
                return -1;
              }

              @Override
              public void willSnapTo(int position, int currentOffsetX, int currentOffsetY,
                  int targetOffsetX, int targetOffsetY) {
                UIListContainer.this.willSnapTo(
                    position, currentOffsetX, currentOffsetY, targetOffsetX, targetOffsetY);
              }
            });
        return;
      }
    }
    mView.mSnapHelper = null;
  }

  public void willSnapTo(
      int position, int currentOffsetX, int currentOffsetY, int targetOffsetX, int targetOffsetY) {
    LynxDetailEvent event = new LynxDetailEvent(getSign(), "snap");
    event.addDetail("position", position);
    event.addDetail("currentScrollLeft", PixelUtils.pxToDip(currentOffsetX));
    event.addDetail("currentScrollTop", PixelUtils.pxToDip(currentOffsetY));
    event.addDetail("targetScrollLeft", PixelUtils.pxToDip(targetOffsetX));
    event.addDetail("targetScrollTop", PixelUtils.pxToDip(targetOffsetY));
    mContext.getEventEmitter().sendCustomEvent(event);
  }

  @LynxProp(name = "experimental-max-fling-distance-ratio")
  public void setMaxFlingDistanceRatio(Dynamic value) {
    if (value.getType() == ReadableType.String && value.asString().equals("auto")) {
      mView.setMaxFlingDistanceRatio(LIST_AUTOMATIC_MAX_FLING_RATIO);
    } else if (value.getType() == ReadableType.Number) {
      mView.setMaxFlingDistanceRatio((float) value.asDouble());
    }
  }

  @LynxProp(name = "enable-scroll", defaultBoolean = true)
  public void setEnableScroll(boolean value) {
    mView.setEnableScroll(value);
  }

  @LynxProp(name = "force-can-scroll", defaultBoolean = false)
  public void setForceCanScroll(boolean value) {
    if (mView != null) {
      mView.setForceCanScroll(value);
    }
  }

  @LynxProp(name = "vertical-orientation", defaultBoolean = false)
  public void setVerticalOrientation(boolean value) {
    mIsVertical = value;
    mView.setOrientation(mIsVertical ? LinearLayout.VERTICAL : LinearLayout.HORIZONTAL);
  }

  @LynxProp(name = "scroll-orientation", customType = "vertical")
  public void setScrollOrientation(String scrollOrientation) {
    if (TextUtils.equals(scrollOrientation, "vertical")) {
      mIsVertical = true;
    } else if (TextUtils.equals(scrollOrientation, "horizontal")) {
      mIsVertical = false;
    } else {
      mIsVertical = true;
    }
    mView.setOrientation(mIsVertical ? LinearLayout.VERTICAL : LinearLayout.HORIZONTAL);
  }

  @LynxProp(name = "enable-nested-scroll", defaultBoolean = false)
  public void setEnableNestedScroll(boolean nestedScroll) {
    mView.setNestedScrollingEnabled(nestedScroll);
  }

  @LynxProp(name = "experimental-batch-render-strategy", defaultInt = 0)
  public void setBatchRenderStrategy(int batchRenderStrategy) {
    mEnableBatchRender = batchRenderStrategy > 0;
  }

  @LynxProp(name = "list-container-info")
  public void setDiffInfo(ReadableMap map) {
    if (map instanceof JavaOnlyMap) {
      JavaOnlyMap listDiffInfo = (JavaOnlyMap) map;

      ReadableArray tempStickyTopIndexes = listDiffInfo.getArray("stickyTop");
      if (tempStickyTopIndexes instanceof JavaOnlyArray) {
        mStickyTopIndexes = ((JavaOnlyArray) tempStickyTopIndexes);
      }

      ReadableArray tempStickyBottomIndexes = listDiffInfo.getArray("stickyBottom");
      if (tempStickyBottomIndexes instanceof JavaOnlyArray) {
        mStickyBottomIndexes = ((JavaOnlyArray) tempStickyBottomIndexes);
      }

      ReadableArray tempItemKeys = listDiffInfo.getArray("itemkeys");
      if (tempItemKeys instanceof JavaOnlyArray) {
        mItemKeys = ((JavaOnlyArray) tempItemKeys);
      }

      // update mItemKeyMap
      final int itemCount = mItemKeys.size();
      mItemKeyMap.clear();
      for (int i = 0; i < itemCount; ++i) {
        mItemKeyMap.put(mItemKeys.getString(i), i);
      }
    }
  }

  @LynxProp(name = "sticky", defaultBoolean = true)
  public void setEnableListSticky(boolean value) {
    // Sticky for horizontal layout is not supported.
    mEnableListSticky = value;
  }

  @LynxProp(name = "sticky-offset", defaultInt = 0)
  public void setStickyOffset(Dynamic value) {
    int offset = ListEventManager.dynamicToInt(value, 0);
    offset = (int) PixelUtils.dipToPx(offset);
    mStickyOffset = offset;
  }

  @LynxProp(name = "enable-fade-in-animation", defaultBoolean = false)
  public void setEnableFadeInAnimation(boolean value) {
    mEnableFadeInAnimation = value;
  }

  @LynxProp(name = "update-animation-fade-in-duration", defaultInt = 100)
  public void setUpdateAnimationFadeInDuration(int value) {
    mUpdateAnimationFadeInDuration = value;
  }

  @LynxProp(name = "need-visible-item-info", defaultBoolean = false)
  public void setNeedVisibleItemInfo(boolean value) {
    mEnableNeedVisibleItemInfo = value;
  }

  @LynxUIMethod
  public void scrollToPosition(ReadableMap params, Callback callback) {
    if (mScrollToCallback != null) {
      // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to null
      // to avoid invoking mScrollToCallback twice.
      Callback lastScrollToCallback = mScrollToCallback;
      mScrollToCallback = null;
      lastScrollToCallback.invoke(LynxUIMethodConstants.UNKNOWN,
          "the scroll has stopped, triggered by a new scrolling request");
    }

    // Perform parameter parsing
    int position = params.getInt("index", params.getInt("position", 0));
    float offset = (float) params.getDouble("offset", 0.0);
    boolean smooth = params.getBoolean("smooth", false);
    int offsetVal = (int) PixelUtils.dipToPx(offset);
    if (position < 0 || position >= mItemKeys.size()) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "position < 0 or position >= data count");
      return;
    }

    // Stop the current scroll
    if (!smooth) {
      mView.stopFling();
    }

    int alignTo = 0;
    String alignToStr = params.getString("alignTo");
    if (TextUtils.equals(alignToStr, "middle")) {
      alignTo = 1;
    } else if (TextUtils.equals(alignToStr, "bottom")) {
      alignTo = 2;
    }

    if (smooth) {
      mScrollToCallback = callback;
    }

    // Tell ListElement that we want scroll to some position
    LynxContext context = getLynxContext();
    ListNodeInfoFetcher listNodeInfoFetcher = null;
    if (context != null) {
      listNodeInfoFetcher = context.getListNodeInfoFetcher();
    }

    if (listNodeInfoFetcher != null) {
      listNodeInfoFetcher.scrollToPosition(getSign(), position, offsetVal, alignTo, smooth);
      if (!smooth) {
        // TODO(xiamengfei.moonface) Invoke callback after ListElement did scroll on Most_On_Tasm
        sendCustomEvent(mView.getScrollX(), mView.getScrollY(), mView.getScrollX(),
            mView.getScrollY(), LynxScrollEvent.EVENT_SCROLL_END);
        callback.invoke(LynxUIMethodConstants.SUCCESS);
      }
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "List has been destroyed");
    }
  }

  @LynxUIMethod
  public void scrollBy(ReadableMap params, Callback callback) {
    if (callback == null) {
      return;
    }
    if (params == null || !params.hasKey("offset")) {
      callback.invoke(
          LynxUIMethodConstants.PARAM_INVALID, "Invoke scrollBy failed due to index param is null");
      return;
    }
    double offset = PixelUtils.dipToPx(params.getDouble("offset", 0));

    // need to post runnable in ui thread, otherwise may trigger npe in dispatchDraw
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      float[] result = scrollBy((float) offset, (float) offset);
      JavaOnlyMap response = new JavaOnlyMap();
      response.putDouble("consumedX", (int) PixelUtils.pxToDip(result[0]));
      response.putDouble("consumedY", (int) PixelUtils.pxToDip(result[1]));
      response.putDouble("unconsumedX", (int) PixelUtils.pxToDip(result[2]));
      response.putDouble("unconsumedY", (int) PixelUtils.pxToDip(result[3]));

      callback.invoke(LynxUIMethodConstants.SUCCESS, response);
    });
  }

  @Override
  protected void consumeGesture(boolean consumeGesture) {
    mView.consumeGesture(consumeGesture);
  }

  @MainThread
  @Override
  public float[] scrollBy(float deltaX, float deltaY) {
    float[] res = new float[4];
    if (mView == null) {
      return res; // Return if mView is null
    }
    int mLastScrollOffsetX = mView.getScrollX();
    int mLastScrollOffsetY = mView.getScrollY();
    if (!mIsVertical) {
      mView.scrollBy((int) deltaX, 0);
    } else {
      mView.scrollBy(0, (int) deltaY);
    }
    // when scroll, not trigger basic events
    if (Math.abs(deltaX) > Float.MIN_VALUE || Math.abs(deltaY) > Float.MIN_VALUE) {
      recognizeGesturere();
    }

    if (!mIsVertical) {
      res[0] = mView.getScrollX() - mLastScrollOffsetX;
      res[1] = 0;
      res[2] = deltaX - res[0];
      res[3] = deltaY;
    } else {
      res[0] = 0;
      res[1] = mView.getScrollY() - mLastScrollOffsetY;
      res[2] = deltaX;
      res[3] = deltaY - res[1];
    }
    return res;
  }

  @LynxUIMethod
  public void getVisibleCells(ReadableMap params, Callback callback) {
    callback.invoke(LynxUIMethodConstants.SUCCESS, visibleCellsInfo());
  }

  private JavaOnlyArray visibleCellsInfo() {
    if (mContext == null) {
      return new JavaOnlyArray();
    }

    List visibleCells = new ArrayList();

    float density = mContext.getScreenMetrics().density;

    int scrollX = mView.getScrollX();
    int scrollY = mView.getScrollY();

    for (int i = 0; i < ((ViewGroup) mView.getLinearLayout()).getChildCount(); i++) {
      View child = ((ViewGroup) mView.getLinearLayout()).getChildAt(i);
      if (child instanceof ComponentView) {
        ComponentView componentView = (ComponentView) child;
        if ((mIsVertical ? isVisibleCellVertical(componentView)
                         : isVisibleCellHorizontal(componentView))) {
          IDrawChildHook drawChildHook = componentView.getDrawChildHook();
          if (drawChildHook instanceof UIComponent) {
            UIComponent component = (UIComponent) drawChildHook;
            JavaOnlyMap map = new JavaOnlyMap();

            map.put("id", component.getIdSelector());
            map.put("position", getIndexFromItemKey(component.getItemKey()));
            map.put("index", getIndexFromItemKey(component.getItemKey()));
            map.put("itemKey", component.getItemKey());
            map.put("top", (child.getTop() - scrollY) / density);
            map.put("bottom", (child.getBottom() - scrollY) / density);
            map.put("left", (child.getLeft() - scrollX) / density);
            map.put("right", (child.getRight() - scrollX) / density);
            visibleCells.add(map);
          }
        }
      }
    }

    Collections.sort(visibleCells, new Comparator<JavaOnlyMap>() {
      @Override
      public int compare(JavaOnlyMap o1, JavaOnlyMap o2) {
        return Integer.compare(o1.getInt("position"), o2.getInt("position"));
      }
    });

    return JavaOnlyArray.from(visibleCells);
  }

  private boolean isVisibleCellVertical(ComponentView view) {
    int minY = view.getTop();
    int maxY = view.getBottom();
    int offsetMinY = mView.getScrollY();
    int offsetMaxY = offsetMinY + mView.getHeight();
    return ((minY <= offsetMinY && maxY >= offsetMinY) || (minY <= offsetMaxY && maxY >= offsetMaxY)
        || (minY >= offsetMinY && maxY <= offsetMaxY));
  }

  private boolean isVisibleCellHorizontal(ComponentView view) {
    int minX = view.getLeft();
    int maxX = view.getRight();
    int offsetMinX = mView.getScrollX();
    int offsetMaxX = offsetMinX + mView.getWidth();
    return ((minX <= offsetMinX && maxX >= offsetMinX) || (minX <= offsetMaxX && maxX >= offsetMaxX)
        || (minX >= offsetMinX && maxX <= offsetMaxX));
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    if (events == null) {
      return;
    }
    mEnableScrollEndEvent = events.containsKey(LynxScrollEvent.EVENT_SCROLL_END);
    mEnableScrollStateChangeEvent = events.containsKey(LynxScrollEvent.EVENT_SCROLL_STATE_CHANGE);
  }

  @LynxUIMethod
  public void autoScroll(ReadableMap params, Callback callback) {
    // TODO(xiamengfei.moonface): Replace the auto-scroll capability in the UIScrollView with the
    // AutoScroller.

    if (mAutoScroller == null) {
      mAutoScroller = new UIListAutoScroller() {
        @Override
        void onAutoScrollError(String msg) {
          callback.invoke(LynxUIMethodConstants.UNKNOWN, msg);
        }

        @Override
        void onAutoScrollStart() {
          mView.mIsDuringAutoScroll = true;
          if (mScrollToCallback != null) {
            // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to
            // null to avoid invoking mScrollToCallback twice.
            Callback lastScrollToCallback = mScrollToCallback;
            mScrollToCallback = null;
            lastScrollToCallback.invoke(LynxUIMethodConstants.PARAM_INVALID,
                "the scroll has stopped, triggered by auto scroll");
          }
          callback.invoke(LynxUIMethodConstants.SUCCESS);
        }

        @Override
        boolean canScroll(int distance) {
          return ((distance > 0 && getView().canScrollBy(1))
              || (distance < 0 && getView().canScrollBy(-1)));
        }

        @Override
        void scrollBy(int distance) {
          if (mIsVertical) {
            getView().scrollBy(0, distance);
          } else {
            getView().scrollBy(distance, 0);
          }
        }

        @Override
        void onAutoScrollEnd() {
          mView.mIsDuringAutoScroll = false;
          mView.setScrollState(SCROLL_STATE_IDLE);
        }
      };
    }
    mAutoScroller.setAutoScrollParams(
        params.getBoolean("start", true), params.getBoolean("autoStop", true));
    mAutoScroller.execute(params.getString("rate", ""), getLynxContext());
    if (mAutoScroller.canScroll(mAutoScroller.mAutoRatePerFrame)) {
      mView.setScrollState(SCROLL_STATE_SCROLL_ANIMATION);
    }
  }

  @Override
  public void onScrollStateChange(NestedScrollContainerView scrollView, int state) {
    // Default behavior:
    // (1) If in fling or dragged status, the component in list will not respond to tap
    // event.
    // (2) If invoke autoScroll, the component in list will respond to tap event.
    recognizeGesturere();
    if (state == SCROLL_STATE_IDLE) {
      getLynxContext().getFluencyTraceHelper().stop(getSign());
      if (mScrollToCallback != null) {
        // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to
        // null to avoid invoking mScrollToCallback twice.
        Callback lastScrollToCallback = mScrollToCallback;
        mScrollToCallback = null;
        lastScrollToCallback.invoke(LynxUIMethodConstants.SUCCESS);
      }
      if (mEnableScrollEndEvent) {
        sendCustomEvent(mView.getScrollX(), mView.getScrollY(), mView.getScrollX(),
            mView.getScrollY(), LynxScrollEvent.EVENT_SCROLL_END);
      }
    } else if (state == SCROLL_STATE_DRAGGING) {
      getLynxContext().getFluencyTraceHelper().start(getSign(), "scroll", getScrollMonitorTag());
      // dragging
      if (mScrollToCallback != null) {
        // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to
        // null to avoid invoking mScrollToCallback twice.
        Callback lastScrollToCallback = mScrollToCallback;
        mScrollToCallback = null;
        lastScrollToCallback.invoke(
            LynxUIMethodConstants.UNKNOWN, "the scroll has stopped, triggered by dragging events");
      }
    }
    if (mEnableScrollStateChangeEvent) {
      LynxDetailEvent event =
          new LynxDetailEvent(getSign(), LynxScrollEvent.EVENT_SCROLL_STATE_CHANGE);

      if (mEnableNeedVisibleItemInfo) {
        event.addDetail("attachedCells", visibleCellsInfo());
      }

      event.addDetail("state", state);
      mContext.getEventEmitter().sendCustomEvent(event);
    }
  }

  public void sendCustomEvent(int l, int t, int oldl, int oldt, String type) {
    LynxScrollEvent event = LynxScrollEvent.createScrollEvent(getSign(), type);
    event.setScrollParams(
        l, t, mView.getMeasuredHeight(), mView.getMeasuredWidth(), l - oldl, t - oldt);
    if (getLynxContext() != null) {
      getLynxContext().getEventEmitter().sendCustomEvent(event);
    }
  }

  public void updateScrollInfo(boolean smooth, float estimatedOffset, boolean scrolling) {
    // ListElement flush scrolling to platform
    mScrollingEstimatedOffset = (int) estimatedOffset;
    if (!scrolling) {
      // Scroll will begin !
      if (mView.getCustomScrollHook() != mCustomScrollHook) {
        mView.setCustomScrollHook(mCustomScrollHook);
      }
      // Trigger scroll
      mView.smoothScrollTo(
          mIsVertical ? 0 : (int) estimatedOffset, mIsVertical ? (int) estimatedOffset : 0);
    }
  }

  private int getIndexFromItemKey(String itemKey) {
    if (TextUtils.isEmpty(itemKey) || !mItemKeyMap.containsKey(itemKey)) {
      return -1;
    }
    return mItemKeyMap.get(itemKey);
  }

  private void updateStickyInfoForInsertedChild(LynxBaseUI child,
      HashMap<Integer, UIComponent> stickyItems, JavaOnlyArray stickyIndexes,
      int indexFromItemKey) {
    if (!mEnableListSticky) {
      return;
    }
    if (child instanceof UIComponent) {
      final int stickyItemCount = stickyIndexes.size();
      for (int i = 0; i < stickyItemCount; ++i) {
        if (indexFromItemKey == stickyIndexes.getInt(i)) {
          stickyItems.put(indexFromItemKey, (UIComponent) child);
          return;
        }
      }
    }
  }

  private void updateStickyInfoForDeletedChild(
      LynxBaseUI child, HashMap<Integer, UIComponent> stickyItems) {
    if (!mEnableListSticky) {
      return;
    }
    if (child instanceof UIComponent) {
      Iterator<Map.Entry<Integer, UIComponent>> iterator = stickyItems.entrySet().iterator();

      while (iterator.hasNext()) {
        Map.Entry<Integer, UIComponent> entry = iterator.next();
        if (entry.getValue() == child) {
          stickyItems.remove(entry.getKey());
          break;
        }
      }
    }
  }

  private void updateStickyInfoForUpdatedChild(LynxBaseUI child,
      HashMap<Integer, UIComponent> stickyItems, JavaOnlyArray stickyIndexes,
      int indexFromItemKey) {
    if (!mEnableListSticky) {
      return;
    }

    // TODO(xiamengfei.moonface): Handle recycle sticky later

    if (child instanceof UIComponent) {
      UIComponent component = (UIComponent) child;
      final int stickyItemCount = stickyIndexes.size();
      for (int i = 0; i < stickyItemCount; ++i) {
        if (indexFromItemKey == stickyIndexes.getInt(i)) {
          stickyItems.put(indexFromItemKey, component);
          return;
        }
      }
    }
  }

  public void updateStickyTops(int offsetTop) {
    if (!mEnableListSticky) {
      return;
    }
    int offset = offsetTop + mStickyOffset;
    UIComponent stickyTopItem = null;
    UIComponent nextStickyTopItem = null;
    // enumerate from bottom to top to find sticky top item
    ListIterator<Object> listIterator = mStickyTopIndexes.listIterator(mStickyTopIndexes.size());
    while (listIterator.hasPrevious()) {
      Integer topIndex = (Integer) listIterator.previous();
      UIComponent top = mStickyTopItems.get(topIndex);
      if (top == null) {
        continue;
      }
      if (top.getTop() > offset) {
        // cache potential next sticky item
        nextStickyTopItem = top;
        // hold its position
        top.getView().setTranslationY(0);
        setChildTranslationZ(top);
      } else if (stickyTopItem != null) {
        // sticky top item found, hold upper sticky top's position
        top.getView().setTranslationY(0);
        setChildTranslationZ(top);
      } else {
        stickyTopItem = top;
      }
    }
    if (stickyTopItem != null) {
      if (mPrevStickyTopItem != stickyTopItem) {
        LynxDetailEvent event = new LynxDetailEvent(getSign(), "stickytop");
        event.addDetail("top", stickyTopItem.getItemKey());
        mContext.getEventEmitter().sendCustomEvent(event);
        mPrevStickyTopItem = stickyTopItem;
      }

      int stickyTopY = offset;
      if (nextStickyTopItem != null) {
        int nextStickyTopItemDistanceFromOffset = nextStickyTopItem.getTop() - offset;
        int squashStickyTopDelta = stickyTopItem.getHeight() - nextStickyTopItemDistanceFromOffset;
        if (squashStickyTopDelta > 0) {
          // need push sticky top item to upper
          stickyTopY -= squashStickyTopDelta;
        }
      }
      if (stickyTopItem.getView() != null) {
        stickyTopItem.getView().setTranslationY(stickyTopY - stickyTopItem.getTop());
        stickyTopItem.getView().bringToFront();
        setChildTranslationZ(stickyTopItem, Integer.MAX_VALUE);
      }
    }
  }

  public void updateStickyBottoms(int offsetTop) {
    if (!mEnableListSticky) {
      return;
    }
    int offset = offsetTop + getHeight() - mStickyOffset;
    UIComponent stickyBottomItem = null;
    UIComponent nextStickyBottomItem = null;
    // enumerate from top to bottom to find sticky top item
    for (Object bottomIndex : mStickyBottomIndexes) {
      UIComponent bottom = mStickyBottomItems.get((Integer) bottomIndex);
      if (bottom == null) {
        continue;
      }
      if (bottom.getTop() + bottom.getHeight() < offset) {
        // cache potential next sticky item
        nextStickyBottomItem = bottom;
        // hold its position
        bottom.getView().setTranslationY(0);
        setChildTranslationZ(bottom);
      } else if (stickyBottomItem != null) {
        // sticky bottom item found, hold upper sticky top's position
        bottom.getView().setTranslationY(0);
        setChildTranslationZ(bottom);
      } else {
        stickyBottomItem = bottom;
      }
    }
    if (stickyBottomItem != null) {
      if (mPrevStickyBottomItem != stickyBottomItem) {
        LynxDetailEvent event = new LynxDetailEvent(getSign(), "stickybottom");
        event.addDetail("bottom", stickyBottomItem.getItemKey());
        mContext.getEventEmitter().sendCustomEvent(event);
        mPrevStickyBottomItem = stickyBottomItem;
      }

      int stickyTopY = offset - stickyBottomItem.getHeight();
      if (nextStickyBottomItem != null) {
        int nextStickyBottomItemDistanceFromOffset =
            offset - (nextStickyBottomItem.getTop() + nextStickyBottomItem.getHeight());
        int squashStickyBottomDelta =
            stickyBottomItem.getHeight() - nextStickyBottomItemDistanceFromOffset;
        if (squashStickyBottomDelta > 0) {
          stickyTopY += squashStickyBottomDelta;
        }
      }
      if (stickyBottomItem.getView() != null) {
        stickyBottomItem.getView().setTranslationY(stickyTopY - stickyBottomItem.getTop());
        stickyBottomItem.getView().bringToFront();
        setChildTranslationZ(stickyBottomItem, Integer.MAX_VALUE);
      }
    }
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    for (int i = ((ViewGroup) mView.getLinearLayout()).getChildCount() - 1; i >= 0; i--) {
      View childView = ((ViewGroup) mView.getLinearLayout()).getChildAt(i);
      if (childView instanceof ComponentView) {
        ComponentView componentView = (ComponentView) childView;
        IDrawChildHook drawChildHook = componentView.getDrawChildHook();
        if (drawChildHook instanceof UIComponent) {
          if (childContainsHitPoint((LynxBaseUI) drawChildHook, x, y, ignoreUserInteraction)) {
            return childHitTest((LynxBaseUI) drawChildHook, x, y, ignoreUserInteraction);
          }
        }
      }
    }
    return this;
  }

  public boolean childContainsHitPoint(
      LynxBaseUI child, float x, float y, boolean ignoreUserInteraction) {
    if (child instanceof UIComponent) {
      return child.containsPoint(x + mView.getScrollX()
              - (mContext.getEnableEventRefactor() ? child.getLeft() + child.getTranslationX() : 0),
          y + mView.getScrollY()
              - (mContext.getEnableEventRefactor() ? child.getTop() + child.getTranslationY() : 0),
          ignoreUserInteraction);
    }
    return false;
  }

  public EventTarget childHitTest(
      LynxBaseUI child, float x, float y, boolean ignoreUserInteraction) {
    return child.hitTest(x + mView.getScrollX() - child.getLeft() - child.getTranslationX(),
        y + mView.getScrollY() - child.getTop() - child.getTranslationY(), ignoreUserInteraction);
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (mGestureHandlers != null) {
      GestureArenaManager manager = getGestureArenaManager();
      // Check if the current UIList instance is already a member of the gesture arena
      if (manager != null && !manager.isMemberExist(getGestureArenaMemberId())) {
        // If not a member, add the UIList instance as a new member to the gesture arena
        mGestureArenaMemberId = manager.addMember(UIListContainer.this);
      }
    }
  }

  @Override
  public void onGestureScrollBy(float x, float y) {
    if (!isEnableNewGesture()) {
      return;
    }
    // need to post runnable in ui thread, otherwise may trigger npe in dispatchDraw
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (mView == null) {
        return;
      }
      if (mIsVertical) {
        mView.scrollBy(0, (int) y);
      } else {
        mView.scrollBy((int) x, 0);
      }
      // when scroll, not trigger basic events
      if (Math.abs(x) > Float.MIN_VALUE || Math.abs(y) > Float.MIN_VALUE) {
        recognizeGesturere();
      }
    });
  }

  @Override
  public boolean canConsumeGesture(float deltaX, float deltaY) {
    // Check if the new gesture feature is enabled and if the event manager is available
    if (!isEnableNewGesture()) {
      return false;
    }

    if (mIsVertical) {
      // For vertical scrolling, check if the gesture is within the valid bounds
      // Return false if the gesture exceeds upper or lower border limits
      return !((isAtBorder(true) && deltaY < 0) || (isAtBorder(false) && deltaY > 0));
    } else {
      // For horizontal scrolling, check if the gesture is within the valid bounds
      // Return false if the gesture exceeds left or right border limits
      return !((isAtBorder(true) && deltaX < 0) || (isAtBorder(false) && deltaX > 0));
    }
  }

  @Override
  public int getMemberScrollX() {
    if (mIsVertical) {
      return 0;
    }
    return mView.getScrollX();
  }

  @Override
  public int getMemberScrollY() {
    if (mIsVertical) {
      return mView.getScrollY();
    } else {
      return 0;
    }
  }

  @Override
  public boolean isAtBorder(boolean isStart) {
    // Check if the new gesture feature is enabled and if the event manager is available
    if (!isEnableNewGesture()) {
      return false;
    }
    if (isStart) {
      return !getView().canScrollBy(-1);
    } else {
      return !getView().canScrollBy(1);
    }
  }

  @Override
  public void onInvalidate() {
    if (!isEnableNewGesture() || mView == null) {
      return;
    }
    ViewCompat.postInvalidateOnAnimation(mView);
  }

  @Override
  public void setGestureDetectors(Map<Integer, GestureDetector> gestureDetectors) {
    super.setGestureDetectors(gestureDetectors);

    // Check if gesture detectors are not available
    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return;
    }

    GestureArenaManager manager = getGestureArenaManager();
    if (manager == null) {
      return;
    }

    // Check if the current UIList instance is already a member of the gesture arena
    if (manager.isMemberExist(getGestureArenaMemberId())) {
      // when update gesture handlers, need to reset it
      if (mGestureHandlers != null) {
        mGestureHandlers.clear();
        mGestureHandlers = null;
      }
    }

    // Lazy initialization of gesture handlers
    if (mGestureHandlers == null && getSign() > 0) {
      // Convert gesture data to gesture handlers if not already initialized
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIListContainer.this, getGestureDetectorMap());
    }
  }

  @Nullable
  @Override
  public Map<Integer, BaseGestureHandler> getGestureHandlers() {
    // Check if the new gesture feature is enabled
    if (!isEnableNewGesture()) {
      return null;
    }

    // Lazy initialization of gesture handlers
    if (mGestureHandlers == null) {
      // Convert gesture data to gesture handlers if not already initialized
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIListContainer.this, getGestureDetectorMap());
    }
    // Return the initialized gesture handlers
    return mGestureHandlers;
  }
}
