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
import com.lynx.tasm.IListNodeInfoFetcher;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.list.LynxSnapHelper;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.behavior.ui.view.ComponentView;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.ui.view.UISimpleView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.event.LynxScrollEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import com.lynx.tasm.gesture.handler.GestureConstants;
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
    implements NestedScrollContainerView.OnScrollStateChangeListener,
               UIComponent.NodeReadyListener {
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
  private boolean mEnableRecycleStickyItem = true;
  private boolean mEnableScrollEndEvent = false;
  private boolean mEnableScrollStateChangeEvent = false;
  private boolean mEnableBatchRender = false;
  private boolean mEnableNeedVisibleItemInfo = false;
  private boolean mUpdateStickyForDiff = true;
  private final HashSet<String> mStickyTopItemKeySet = new HashSet<>();
  private final HashSet<String> mStickyBottomItemKeySet = new HashSet<>();
  private final HashMap<String, UIComponent> mStickyTopItemMap = new HashMap<>();
  private final HashMap<String, UIComponent> mStickyBottomItemMap = new HashMap<>();
  private Callback mScrollToCallback = null;
  private boolean mEnableInsertPlatformViewOperation = false;
  private int mScrollingEstimatedOffset = INVALID_SCROLL_ESTIMATED_OFFSET;

  private ListContainerProxy mListContainerProxy = null;

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

          if (mListContainerProxy != null) {
            mListContainerProxy.scrollStopped(getSign());
          } else {
            if (context != null && context.getListNodeInfoFetcher() != null) {
              getLynxContext().getListNodeInfoFetcher().scrollStopped(getSign());
            }
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
    this(context, null);
  }

  public UIListContainer(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected ListContainerView createView(Context context) {
    final ListContainerView listContainerView = new ListContainerView(context, this);
    listContainerView.addOnScrollStateChangeListener(this);
    LLog.i(TAG, "create UIListContainer: " + this + ", " + listContainerView);
    return listContainerView;
  }

  @Override
  public boolean isScrollContainer() {
    return true;
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    // The list container should manager the view of component on the interface of
    // "onLayoutFinish()"
    super.onInsertChild(child, index);
    if (mEnableListSticky && !mUpdateStickyForDiff) {
      int indexFromItemKey = getIndexFromItemKey(((UIComponent) child).getItemKey());
      updateStickyInfoForInsertedChild(child, mStickyTopItems, mStickyTopIndexes, indexFromItemKey);
      updateStickyInfoForInsertedChild(
          child, mStickyBottomItems, mStickyBottomIndexes, indexFromItemKey);
    }
  }

  public void onComponentNodeReady(UIComponent component) {
    // This callback is invoked by component's onNodeReady(), which means component has
    // valid item-key info, so handle component's item-key changed for sticky.
    if (mEnableListSticky && mUpdateStickyForDiff && component != null
        && component.getItemKey() != null) {
      String itemKey = component.getItemKey();
      if (mStickyTopItemKeySet.contains(itemKey)) {
        // Update sticky top list item map.
        updateStickyItemMap(component, mStickyTopItemMap, true);
      } else if (mStickyBottomItemKeySet.contains(itemKey)) {
        // Update sticky bottom list item map.
        updateStickyItemMap(component, mStickyBottomItemMap, true);
      } else {
        // Not sticky top or bottom list item, remove it from map.
        updateStickyItemMap(component, mStickyTopItemMap, false);
        updateStickyItemMap(component, mStickyBottomItemMap, false);
      }
    }
  }

  private void updateStickyItemMap(
      UIComponent component, HashMap<String, UIComponent> stickyItemMap, boolean isStickyItem) {
    if (component != null && component.getItemKey() != null) {
      if (isStickyItem) {
        String newUpdatedItemKey = component.getItemKey();
        if (stickyItemMap.get(newUpdatedItemKey) == component) {
          // No need to update sticky item map.
          return;
        }
        Iterator<Map.Entry<String, UIComponent>> iterator = stickyItemMap.entrySet().iterator();
        while (iterator.hasNext()) {
          Map.Entry<String, UIComponent> entry = iterator.next();
          if (entry.getValue() == component
              && !TextUtils.equals(entry.getKey(), newUpdatedItemKey)) {
            // Delete old and insert new <item-key, list-item> pair to finish updating item-key.
            iterator.remove();
            stickyItemMap.put(newUpdatedItemKey, component);
            break;
          }
        }
      } else {
        // The component is not sticky top or bottom list item, remove it from map.
        Iterator<Map.Entry<String, UIComponent>> iterator = stickyItemMap.entrySet().iterator();
        while (iterator.hasNext()) {
          Map.Entry<String, UIComponent> entry = iterator.next();
          if (entry.getValue() == component) {
            // Delete old <item-key, list-item> pair.
            iterator.remove();
            resetStickyItem(component);
            break;
          }
        }
      }
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
      component.setNodeReadyListener(null);
    }
    if (mEnableListSticky && !mUpdateStickyForDiff) {
      updateStickyInfoForDeletedChild(child, mStickyTopItems);
      updateStickyInfoForDeletedChild(child, mStickyBottomItems);
    }
  }

  @Override
  public void onLayoutFinish(long operationId, @Nullable LynxBaseUI component) {
    super.onLayoutFinish(operationId, component);
    if (!mEnableBatchRender && !mEnableInsertPlatformViewOperation) {
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
        if (mUpdateStickyForDiff) {
          // This method is invoked in FinishLayoutOperation or by c++ list element which means
          // component has valid item-key info.
          String itemKey = child.getItemKey();
          if (itemKey != null) {
            // Add <item-key, list-item> to map if current component is sticky item and add list as
            // component node ready listener.
            if (mStickyTopItemKeySet.contains(itemKey)) {
              mStickyTopItemMap.put(itemKey, child);
              child.setNodeReadyListener(this);
            } else if (mStickyBottomItemKeySet.contains(itemKey)) {
              mStickyBottomItemMap.put(itemKey, child);
              child.setNodeReadyListener(this);
            }
          }
        } else {
          int indexFromItemKey = getIndexFromItemKey(child.getItemKey());
          updateStickyInfoForUpdatedChild(
              child, mStickyTopItems, mStickyTopIndexes, indexFromItemKey);
          updateStickyInfoForUpdatedChild(
              child, mStickyBottomItems, mStickyBottomIndexes, indexFromItemKey);
        }
      }
    }
  }

  @Override
  public void removeView(LynxBaseUI child) {
    super.removeView(child);
    if (child instanceof UIComponent) {
      UIComponent component = (UIComponent) child;
      if (mEnableListSticky) {
        if (mUpdateStickyForDiff) {
          // Remove <item-key, list-item> from map if current component is sticky item. Note: need
          // set node ready listener to null because this component may be reused by any no sticky
          // item.
          String itemKey = component.getItemKey();
          if (itemKey != null) {
            if (mStickyTopItemMap.get(itemKey) == component) {
              mStickyTopItemMap.remove(itemKey);
              component.setNodeReadyListener(null);
              if (mEnableRecycleStickyItem) {
                resetStickyItem(component);
              }
            } else if (mStickyBottomItemMap.get(itemKey) == component) {
              mStickyBottomItemMap.remove(itemKey);
              component.setNodeReadyListener(null);
              if (mEnableRecycleStickyItem) {
                resetStickyItem(component);
              }
            }
          }
        } else {
          updateStickyInfoForDeletedChild(component, mStickyTopItems);
          updateStickyInfoForDeletedChild(component, mStickyBottomItems);
        }
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
    initListContainerProxy();
    updateStickyStarts();
    updateStickyEnds();
  }

  private void initListContainerProxy() {
    if (mListContainerProxy == null) {
      if (getLynxContext() != null && getLynxContext().getListNodeInfoFetcher() != null
          && getLynxContext().getListNodeInfoFetcher().getListEngineProxy() != 0) {
        IListNodeInfoFetcher listNodeInfoFetcher = getLynxContext().getListNodeInfoFetcher();
        mListContainerProxy =
            new ListContainerProxy(this, listNodeInfoFetcher.getListEngineProxy());
      }
    }
  }

  public ListContainerProxy getListContainerProxy() {
    return mListContainerProxy;
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    // Note: In the scrollview nested list case, scrollview will be set the layout direction to RTL,
    // so the ListContainerView will inheriting the RTL layout from its parent view. But the RTL
    // layout of the list is implemented on the C++ side, so the ListContainerView view needs to
    // enforce an LTR layout.
    ViewCompat.setLayoutDirection(mView, ViewCompat.LAYOUT_DIRECTION_LTR);
    if (mIsVertical && mView.mMeasuredWidth != getWidth()) {
      mView.setMeasuredSize(getWidth(), mView.mMeasuredHeight);
    } else if (!mIsVertical && mView.mMeasuredHeight != getHeight()) {
      mView.setMeasuredSize(mView.mMeasuredWidth, getHeight());
    }
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
  public void layout() {
    // Note: If list is overflow visible, mView's parent will be set clip children to false in
    // super.layout() which lets mView not be clipped to its bound. Consider list item view's parent
    // is the LinearLayout, so we should also invoke mView.setClipChildren(false) to make
    // LinearLayout not be clipped to its bound.
    if (getOverflow() != OVERFLOW_HIDDEN) {
      mView.setClipChildren(false);
    }
    super.layout();
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
    LLog.i(TAG, "destroy: " + this + ", listContainerView: " + mView);
    super.destroy();
    if (mView != null) {
      mView.destroy();
    }
    if (mAutoScroller != null) {
      mAutoScroller.removeFrameCallback();
    }

    if (mListContainerProxy != null) {
      mListContainerProxy.destroy();
      mListContainerProxy = null;
    }
  }

  public void updateContentSizeAndOffset(float contentSize, float deltaX, float deltaY) {
    if (DEBUG) {
      LLog.i(TAG, "updateContentSizeAndOffset: " + contentSize + ", " + deltaX + ", " + deltaY);
    }
    mView.updateContentSizeAndOffset((int) contentSize, (int) deltaX, (int) deltaY);
  }

  @LynxProp(name = "item-snap")
  public void setPagingAlignment(ReadableMap params) {
    if (params != null && params.size() > 0) {
      double factor = params.getDouble("factor");
      if (factor < 0 || factor > 1) {
        getLynxContext().handleLynxError(
            new LynxError(LynxSubErrorCode.E_COMPONENT_LIST_INVALID_PROPS_ARG, "item-snap invalid!",
                "The factor should be constrained to the range of [0,1].", LynxError.LEVEL_WARN));
        factor = 0;
      }
      int offset = params.getInt("offset", 0);
      if (Math.abs(offset) > 0) {
        // The unit of offset is changed from ppx to px on Android platform in Lynx SDK version 3.7.
        getLynxContext().handleLynxError(new LynxError(
            LynxSubErrorCode.E_COMPONENT_LIST_INVALID_PROPS_ARG,
            "The unit of offset is changed from ppx to px on Android platform in Lynx SDK version 3.7. If Lynx SDK version < 3.7, you can use Math.round(offset * PixelRatio).",
            "", LynxError.LEVEL_WARN));
        offset = (int) PixelUtils.dipToPx(offset);
      }
      // The logic here is copied form the SnapHelper provided by Android.
      // TODO: Speed up the snap. In the future, the easing-curve should be able to be customized.
      double snapAlignmentMillisecondsPerPx = 50f / mContext.getScreenMetrics().densityDpi;
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
            public int getContentHeight() {
              return getView().getLinearLayout().getHeight();
            }

            @Override
            public int getContentWidth() {
              return getView().getLinearLayout().getWidth();
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
  public void setDiffInfo(ReadableMap listContainerInfo) {
    if (listContainerInfo != null) {
      ReadableArray tempStickyStartIndexes = listContainerInfo.getArray("stickyStart");
      if (tempStickyStartIndexes != null) {
        // Note: Here we should copy from ReadableArray to JavaOnlyArray for adapting other
        // framework.
        mStickyTopIndexes = JavaOnlyArray.shallowCopy(tempStickyStartIndexes);
      }
      ReadableArray tempStickyEndIndexes = listContainerInfo.getArray("stickyEnd");
      if (tempStickyEndIndexes != null) {
        mStickyBottomIndexes = JavaOnlyArray.shallowCopy(tempStickyEndIndexes);
      }
      ReadableArray tempItemKeys = listContainerInfo.getArray("itemkeys");
      if (tempItemKeys != null) {
        mItemKeys = JavaOnlyArray.shallowCopy(tempItemKeys);
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

  @LynxProp(name = "sticky-offset", defaultFloat = 0)
  public void setStickyOffset(float value) {
    mStickyOffset = (int) PixelUtils.dipToPx(value);
  }

  @LynxProp(name = "enable-fade-in-animation", defaultBoolean = false)
  public void setEnableFadeInAnimation(boolean value) {
    mEnableFadeInAnimation = value;
  }

  @LynxProp(name = "enable-insert-platform-view-operation", defaultBoolean = false)
  public void setEnableInsertPlatformViewOperation(boolean value) {
    mEnableInsertPlatformViewOperation = value;
  }

  @LynxProp(name = "update-animation-fade-in-duration", defaultInt = 100)
  public void setUpdateAnimationFadeInDuration(int value) {
    mUpdateAnimationFadeInDuration = value;
  }

  @LynxProp(name = "experimental-recycle-sticky-item", defaultBoolean = true)
  public void setEnableRecycleStickyItem(boolean value) {
    mEnableRecycleStickyItem = value;
  }

  @LynxProp(name = "need-visible-item-info", defaultBoolean = false)
  public void setNeedVisibleItemInfo(boolean value) {
    mEnableNeedVisibleItemInfo = value;
  }

  @LynxProp(name = "experimental-update-sticky-for-diff", defaultBoolean = true)
  public void setUpdateStickyForDiff(boolean value) {
    mUpdateStickyForDiff = value;
  }

  /**
   * get list's scroll info
   * @param callback
   * @return scrollX / scrollY - content offset
   */
  @LynxUIMethod
  public void getScrollInfo(Callback callback) {
    int scrollX = getMemberScrollX();
    int scrollY = getMemberScrollY();
    JavaOnlyMap result = new JavaOnlyMap();
    result.putInt("scrollX", LynxUIHelper.px2dip(mContext, scrollX));
    result.putInt("scrollY", LynxUIHelper.px2dip(mContext, scrollY));
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
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
    int positionParam = params.getInt("index", params.getInt("position", 0));
    String itemKey = params.getString("itemKey");
    int resolvedPosition = positionParam;
    if (!TextUtils.isEmpty(itemKey)) {
      Integer idx = mItemKeyMap.get(itemKey);
      if (idx != null) {
        resolvedPosition = idx;
      }
    }

    float offset = (float) params.getDouble("offset", 0.0);
    boolean smooth = params.getBoolean("smooth", false);
    int offsetVal = (int) PixelUtils.dipToPx(offset);
    if (resolvedPosition < 0 || resolvedPosition >= mItemKeys.size()) {
      callback.invoke(
          LynxUIMethodConstants.OPERATION_ERROR, "position < 0 or position >= data count");
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
    IListNodeInfoFetcher listNodeInfoFetcher = null;

    if (mListContainerProxy != null) {
      mListContainerProxy.scrollToPosition(getSign(), resolvedPosition, offsetVal, alignTo, smooth);
    } else if (context != null && context.getListNodeInfoFetcher() != null) {
      listNodeInfoFetcher = context.getListNodeInfoFetcher();
      listNodeInfoFetcher.scrollToPosition(getSign(), resolvedPosition, offsetVal, alignTo, smooth);
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "List has been destroyed");
      return;
    }
    if (!smooth) {
      sendCustomEvent(mView.getScrollX(), mView.getScrollY(), mView.getScrollX(),
          mView.getScrollY(), LynxScrollEvent.EVENT_SCROLL_END);
      callback.invoke(LynxUIMethodConstants.SUCCESS);
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

  @Override
  protected void interceptGesture(boolean interceptGesture) {
    mView.interceptGesture(interceptGesture);
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
          if (canScroll(mAutoRatePerFrame)) {
            UIListContainer.this.mView.setScrollState(SCROLL_STATE_SCROLL_ANIMATION);
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
  }

  @Override
  public void onScrollStateChange(NestedScrollContainerView scrollView, int state) {
    // Default behavior:
    // (1) If in fling or dragged status, the component in list will not respond to tap
    // event.
    // (2) If invoke autoScroll, the component in list will respond to tap event.
    if (state != SCROLL_STATE_IDLE) {
      recognizeGesturere();
    }
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
          if (mEnableRecycleStickyItem) {
            resetStickyItem((UIComponent) child);
          }
          iterator.remove();
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

  private void resetStickyItem(UIComponent component) {
    if (component.getView() != null) {
      if (mIsVertical) {
        component.getView().setTranslationY(0);
      } else {
        component.getView().setTranslationX(0);
      }

      setChildTranslationZ(component);
    }
  }

  private UIComponent getStickyItemWithIndex(Integer indexValue, boolean isStickyTop) {
    UIComponent component = null;
    if (mUpdateStickyForDiff) {
      HashMap<String, UIComponent> stickyItemMap =
          isStickyTop ? mStickyTopItemMap : mStickyBottomItemMap;
      int index = indexValue.intValue();
      if (index >= 0 && index < mItemKeys.size()) {
        String itemKey = mItemKeys.getString(index);
        if (itemKey != null) {
          component = stickyItemMap.get(itemKey);
        }
      }
    } else {
      HashMap<Integer, UIComponent> stickyItemMap =
          isStickyTop ? mStickyTopItems : mStickyBottomItems;
      component = stickyItemMap.get(indexValue);
    }
    return component;
  }

  public void updateStickyStarts() {
    if (!mEnableListSticky) {
      return;
    }
    int offset = (mIsVertical ? getScrollY() : getScrollX()) + mStickyOffset;
    UIComponent stickyStartItem = null;
    UIComponent nextStickyStartItem = null;
    // enumerate from bottom to top to find sticky top item
    ListIterator<Object> listIterator = mStickyTopIndexes.listIterator(mStickyTopIndexes.size());
    while (listIterator.hasPrevious()) {
      Integer startIndex = (Integer) listIterator.previous();
      UIComponent startComponent = getStickyItemWithIndex(startIndex, true);
      if (startComponent == null) {
        continue;
      }
      int curComponentOffset = mIsVertical ? startComponent.getTop() : startComponent.getLeft();
      if (curComponentOffset > offset) {
        // cache potential next sticky item
        nextStickyStartItem = startComponent;
        // hold its position
        resetStickyItem(startComponent);
      } else if (stickyStartItem != null) {
        // sticky startComponent item found, hold upper sticky startComponent's position
        resetStickyItem(startComponent);
      } else {
        stickyStartItem = startComponent;
      }
    }
    if (stickyStartItem != null) {
      if (mPrevStickyTopItem != stickyStartItem) {
        if (mIsVertical) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), "stickytop");
          event.addDetail("top", stickyStartItem.getItemKey());
          mContext.getEventEmitter().sendCustomEvent(event);
        }
        LynxDetailEvent startEvent = new LynxDetailEvent(getSign(), "stickystart");
        startEvent.addDetail("start", stickyStartItem.getItemKey());
        mContext.getEventEmitter().sendCustomEvent(startEvent);

        mPrevStickyTopItem = stickyStartItem;
      }

      int stickyStartOffset = offset;
      if (nextStickyStartItem != null) {
        int nextStickyStartItemDistanceFromOffset = 0;
        int squashStickyStartDelta = 0;

        if (mIsVertical) {
          nextStickyStartItemDistanceFromOffset = nextStickyStartItem.getTop() - offset;
          squashStickyStartDelta =
              stickyStartItem.getHeight() - nextStickyStartItemDistanceFromOffset;
        } else {
          nextStickyStartItemDistanceFromOffset = nextStickyStartItem.getLeft() - offset;
          squashStickyStartDelta =
              stickyStartItem.getWidth() - nextStickyStartItemDistanceFromOffset;
        }

        if (squashStickyStartDelta > 0) {
          // need push sticky top item to upper
          stickyStartOffset -= squashStickyStartDelta;
        }
      }
      if (stickyStartItem.getView() != null) {
        if (mIsVertical) {
          stickyStartItem.getView().setTranslationY(stickyStartOffset - stickyStartItem.getTop());
        } else {
          stickyStartItem.getView().setTranslationX(stickyStartOffset - stickyStartItem.getLeft());
        }
        stickyStartItem.getView().bringToFront();
        setChildTranslationZ(stickyStartItem, Integer.MAX_VALUE);
      }
    }
  }

  public void updateStickyEnds() {
    if (!mEnableListSticky) {
      return;
    }
    int offset = 0;
    if (mIsVertical) {
      offset = getHeight() + getScrollY() - mStickyOffset;
    } else {
      offset = getWidth() + getScrollX() - mStickyOffset;
    }
    UIComponent stickyEndItem = null;
    UIComponent nextStickyEndItem = null;
    // enumerate from top to bottom to find sticky top item
    for (Object EndIndex : mStickyBottomIndexes) {
      UIComponent endComponent = getStickyItemWithIndex((Integer) EndIndex, false);
      if (endComponent == null) {
        continue;
      }
      int currentComponentOffset = mIsVertical ? (endComponent.getTop() + endComponent.getHeight())
                                               : (endComponent.getLeft() + endComponent.getWidth());
      if (currentComponentOffset < offset) {
        // cache potential next sticky item
        nextStickyEndItem = endComponent;
        // hold its position
        resetStickyItem(endComponent);
      } else if (stickyEndItem != null) {
        // sticky endComponent item found, hold upper sticky top's position
        resetStickyItem(endComponent);
      } else {
        stickyEndItem = endComponent;
      }
    }

    if (stickyEndItem != null) {
      if (mPrevStickyBottomItem != stickyEndItem) {
        if (mIsVertical) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), "stickybottom");
          event.addDetail("bottom", stickyEndItem.getItemKey());
          mContext.getEventEmitter().sendCustomEvent(event);
        }

        LynxDetailEvent endEvent = new LynxDetailEvent(getSign(), "stickyend");
        endEvent.addDetail("end", stickyEndItem.getItemKey());
        mContext.getEventEmitter().sendCustomEvent(endEvent);

        mPrevStickyBottomItem = stickyEndItem;
      }

      int stickyLeftOffset =
          offset - (mIsVertical ? stickyEndItem.getHeight() : stickyEndItem.getWidth());
      if (nextStickyEndItem != null) {
        int nextStickyEndItemDistanceFromOffset = 0;
        int squashStickyEndDelta = 0;
        if (mIsVertical) {
          nextStickyEndItemDistanceFromOffset =
              offset - (nextStickyEndItem.getTop() + nextStickyEndItem.getHeight());
          squashStickyEndDelta = stickyEndItem.getHeight() - nextStickyEndItemDistanceFromOffset;
        } else {
          nextStickyEndItemDistanceFromOffset =
              offset - (nextStickyEndItem.getLeft() + nextStickyEndItem.getWidth());
          squashStickyEndDelta = stickyEndItem.getWidth() - nextStickyEndItemDistanceFromOffset;
        }

        if (squashStickyEndDelta > 0) {
          stickyLeftOffset += squashStickyEndDelta;
        }
      }
      if (stickyEndItem.getView() != null) {
        if (mIsVertical) {
          stickyEndItem.getView().setTranslationY(stickyLeftOffset - stickyEndItem.getTop());
        } else {
          stickyEndItem.getView().setTranslationX(stickyLeftOffset - stickyEndItem.getLeft());
        }
        stickyEndItem.getView().bringToFront();
        setChildTranslationZ(stickyEndItem, Integer.MAX_VALUE);
      }
    }
  }

  private void generateStickyItemKeySet(HashSet<String> stickyItemKeySet,
      JavaOnlyArray stickyItemIndexes, HashMap<String, UIComponent> stickyItemMap) {
    stickyItemKeySet.clear();
    final int stickyItemCount = stickyItemIndexes.size();
    for (int i = 0; i < stickyItemCount; ++i) {
      int index = stickyItemIndexes.getInt(i);
      if (index >= 0 && index < mItemKeys.size()) {
        stickyItemKeySet.add(mItemKeys.getString(index));
      }
    }
    // Remove item from sticky dict if not sticky.
    Iterator<Map.Entry<String, UIComponent>> iterator = stickyItemMap.entrySet().iterator();
    while (iterator.hasNext()) {
      Map.Entry<String, UIComponent> entry = iterator.next();
      String itemKey = entry.getKey();
      UIComponent component = entry.getValue();
      if (component != null && itemKey != null && !stickyItemKeySet.contains(itemKey)) {
        resetStickyItem(component);
        component.setNodeReadyListener(null);
        iterator.remove();
      }
    }
  }

  @Override
  public int getScrollX() {
    return mView.getScrollX();
  }

  @Override
  public int getScrollY() {
    return mView.getScrollY();
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (mEnableListSticky && mUpdateStickyForDiff) {
      // Generate sticky top/bottom item key set.
      generateStickyItemKeySet(mStickyTopItemKeySet, mStickyTopIndexes, mStickyTopItemMap);
      generateStickyItemKeySet(mStickyBottomItemKeySet, mStickyBottomIndexes, mStickyBottomItemMap);
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
  public int getScrollContainerDirection() {
    return mIsVertical ? GestureConstants.DIRECTION_VERTICAL
                       : GestureConstants.DIRECTION_HORIZONTAL;
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
  }

  @Nullable
  @Override
  public Map<Integer, BaseGestureHandler> getGestureHandlers() {
    return super.getGestureHandlers();
  }

  @Override
  public void setPanInterceptSelf(boolean panInterceptSelf) {
    mView.setPanInterceptSelf(panInterceptSelf);
  }

  @Override
  public void setPanInterceptAncestors(boolean panInterceptAncestors) {
    mView.setPanInterceptAncestors(panInterceptAncestors);
  }

  @Override
  public void setPanInterceptDescendants(boolean panInterceptDescendants) {
    mView.setPanInterceptDescendants(panInterceptDescendants);
  }
}
