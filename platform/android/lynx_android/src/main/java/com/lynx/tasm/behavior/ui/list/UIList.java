// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import static android.view.View.LAYOUT_DIRECTION_LTR;
import static android.view.View.LAYOUT_DIRECTION_RTL;
import static android.view.View.OVER_SCROLL_NEVER;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.Rect;
import android.text.TextUtils;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.math.MathUtils;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.PagerSnapHelper;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
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
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityDelegate;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.accessibility.LynxNodeProvider;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.event.LynxListEvent;
import com.lynx.tasm.featurecount.LynxFeatureCounter;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class UIList extends AbsLynxList<RecyclerView> implements GestureArenaMember {
  private UIListAdapter mAdapter;
  /* package */ int mColumnCount = 1;
  private int mMainAxisGap = 0;
  private int mCrossAxisGap = 0;
  private @NonNull String mListType = LIST_TYPE_SINGLE;
  protected boolean mNeedUpdateLayoutManager = true;
  /* package */ ListEventManager mListEventManager;
  private ListScroller mListScroller;
  private boolean mAutoMeasure = false;
  private boolean mNeedLayoutComplete = false;
  private SnapHelper mSnapHelper;
  private FactoredPagerSnapHelper mFactorSnapHelper;
  /* package */ boolean mEnableScroll = true;
  boolean noInvalidate = false;
  private static final int DIRECTION_UP_OR_LEFT = -1;
  private static final int DIRECTION_DOWN_OR_RIGHT = 1;

  // for new gesture
  private Map<Integer, BaseGestureHandler> mGestureHandlers;

  private ViewGroup mContainerView;
  private ListStickyManager mListStickyManager;
  private AppearEventCourier mAppearEventCourier;
  private int mInitialScrollIndex = RecyclerView.NO_POSITION;
  private int mPendingStickyOffset = 0;
  private boolean mPendingOldStickCategory = true;
  private boolean mNewScrollTop = false;
  private boolean mVerticalOrientation = true;
  public Map<String, Object> nativeListStateCache = new HashMap<>();
  public Map<String, HashSet<String>> initialFlushPropCache = new HashMap<>();
  private ReadableMap mListPlatformInfo;
  private ArrayList<String> mComponentAccessibilityOrder = new ArrayList<>();
  private HashMap<String, ArrayList<LynxNodeProvider.LynxCustomNodeInfo>> mCustomNodeMap =
      new HashMap<>();
  private ReadableMap mListNoDiffInfo;
  private boolean mNewArch;
  public static final String TAG = "UIList";
  public static boolean DEBUG = false;
  private boolean mUpperLowerSwitch = false;
  private boolean mStart = false;
  private int mAutoRatePerFrame = 0;
  private boolean mAutoStopOnBounds = true;
  private boolean mTriggerStickyLayout = false;
  private boolean mEnableItemPrefetch = true;
  boolean mIgnoreAttachForBinding = false;
  private boolean mEnableAsyncList = false;
  private boolean mEnableRTL = false;
  boolean mEnableSizeCache = false;
  ListPreloadCache mPreloadCache;
  int mPreloadBufferCount = 0;
  protected boolean mEnableGapItemDecoration = false;
  private GapItemDecoration mGapItemDecoration = null;

  static int ITEM_HOLDER_TYPE_DEFAULT = 0;
  static int ITEM_HOLDER_TYPE_CLEAR = 1;
  static int ITEM_HOLDER_TYPE_KEEP = 2;

  public int mItemHolderType = ITEM_HOLDER_TYPE_DEFAULT;

  private float mMaxFlingVelocityPercent = 1.0f;
  private RecyclerView.OnScrollListener mPreloadListener = null;
  private boolean mEnableStrictScrollable = false;
  private boolean mEnableFocusSearch = true;
  private boolean mStackFromEnd = false;
  private boolean mFiberArch = false;
  private Choreographer.FrameCallback mFrameCallback = null;

  private int mScrollIndex = -1;
  // TODO(dingwang.wxx): In 2.17, the default value of mEnableOverflow will be set to true.
  private boolean mEnableOverflow = false;

  public UIList(LynxContext context) {
    super(context);
    if (DEBUG) {
      LLog.i(TAG, "UIList init");
    }
  }

  @Override
  protected RecyclerView createView(Context context) {
    RecyclerView recyclerView = createRecyclerView(context);
    // make padding not clip header or footer
    recyclerView.setClipToPadding(false);
    RecyclerView.RecycledViewPool pool = new RecyclerView.RecycledViewPool() {
      @Override
      public void putRecycledView(RecyclerView.ViewHolder scrap) {
        if (isAsyncThreadStrategy() && scrap instanceof ListViewHolder) {
          ListViewHolder holder = (ListViewHolder) scrap;
          if (holder.getUIComponent() != null) {
            if (mPreloadCache != null && mPreloadBufferCount > 0) {
              mPreloadCache.enqueueComponentFromRecyclerPool(holder);
            } else {
              mAdapter.recycleHolderComponent(holder);
            }
          }
        }
        super.putRecycledView(scrap);
      }
    };
    recyclerView.setRecycledViewPool(pool);
    mListEventManager =
        new ListEventManager(getLynxContext().getEventEmitter(), recyclerView, this);
    mAppearEventCourier = new AppearEventCourier(getLynxContext().getEventEmitter(), recyclerView);
    recyclerView.setItemAnimator(null);
    mAdapter = new UIListAdapter(this, mAppearEventCourier);
    mListScroller = new ListScroller(context, recyclerView);
    recyclerView.setOverScrollMode(OVER_SCROLL_NEVER);

    if (isAsyncThreadStrategy()) {
      // Add feature counter to count when this async-list is set to true
      LynxFeatureCounter.count(LynxFeatureCounter.JAVA_ENABLE_ASYNC_LIST, mContext.getInstanceId());
    }

    return recyclerView;
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
      mView.scrollBy((int) x, (int) y);
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

    if (isVertical()) {
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
  public boolean isAtBorder(boolean isStart) {
    // Check if the new gesture feature is enabled and if the event manager is available
    if (!isEnableNewGesture()) {
      return false;
    }
    if (isStart) {
      return !canScroll(DIRECTION_UP_OR_LEFT);
    } else {
      return !canScroll(DIRECTION_DOWN_OR_RIGHT);
    }
  }

  @Override
  public void onInvalidate() {
    if (!isEnableNewGesture() || mView == null) {
      return;
    }
    ViewCompat.postInvalidateOnAnimation(mView);
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
          getSign(), getLynxContext(), UIList.this, getGestureDetectorMap());
    }
    // Return the initialized gesture handlers
    return mGestureHandlers;
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
          getSign(), getLynxContext(), UIList.this, getGestureDetectorMap());
    }
  }

  // Override dispatchNestedPreScroll & dispatchNestedPreScroll function to detect nested-scroll
  // outside LynxView.
  private static class PrivateRecyclerView extends RecyclerView {
    private WeakReference<LynxContext> mWeakContext = null;
    private WeakReference<UIList> mWeakUIList = null;
    private boolean mTouchScroll = true;
    protected boolean mPreferenceConsumeGesture = false;
    private boolean mEnableOverflow = false;
    private ScrollContainerDrawHelper mDrawHelper = new ScrollContainerDrawHelper();
    // Whether to consume gestures
    private Boolean mConsumeGesture = null;
    // Whether the down event has been processed, gesture starts from the down event, so if you want
    // to handle the gesture with one gesture, you need to convert one of the move events into a
    // down event
    private boolean mIsDownEventHandled = true;

    public PrivateRecyclerView(Context context, UIList ui) {
      super(context);
      if (context != null && context instanceof LynxContext) {
        mWeakContext = new WeakReference<>((LynxContext) context);
        mWeakUIList = new WeakReference<>(ui);
      }
    }

    @Override
    protected void onAttachedToWindow() {
      LLog.i(UIList.TAG, "PrivateRecyclerView onAttachedToWindow");
      super.onAttachedToWindow();
    }

    @Override
    protected void onDetachedFromWindow() {
      LLog.i(UIList.TAG, "PrivateRecyclerView onDetachedFromWindow");
      super.onDetachedFromWindow();
    }

    @Override
    public void setClipBounds(Rect clipBounds) {
      if (!mEnableOverflow) {
        super.setClipBounds(clipBounds);
        return;
      }
      mDrawHelper.setUiBound(clipBounds);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
      if (!mEnableOverflow) {
        super.dispatchDraw(canvas);
        return;
      }
      Path clipPath = mDrawHelper.getClipPath(this);
      int count = canvas.save();
      if (clipPath != null) {
        canvas.clipPath(clipPath);
      }
      super.dispatchDraw(canvas);
      canvas.restoreToCount(count);
    }

    public void setEnableOverflow(boolean value) {
      mEnableOverflow = value;
    }

    @Override
    public boolean fling(int velocityX, int velocityY) {
      UIList list = mWeakUIList.get();
      if (list == null) {
        super.fling(velocityX, velocityY);
      }

      if (list.mMaxFlingVelocityPercent > 0 && list.mMaxFlingVelocityPercent < 1) {
        if (list.mVerticalOrientation) {
          velocityY = (int) MathUtils.clamp(velocityY,
              -getMaxFlingVelocity() * list.mMaxFlingVelocityPercent,
              getMaxFlingVelocity() * list.mMaxFlingVelocityPercent);
        } else {
          velocityX = (int) MathUtils.clamp(velocityX,
              -getMaxFlingVelocity() * list.mMaxFlingVelocityPercent,
              getMaxFlingVelocity() * list.mMaxFlingVelocityPercent);
        }
      }
      return super.fling(velocityX, velocityY);
    }

    @Override
    public void computeScroll() {
      super.computeScroll();
      UIList list = mWeakUIList.get();
      if (list == null) {
        return;
      }
      if (!list.isEnableNewGesture()) {
        return;
      }
      GestureArenaManager manager = list.getGestureArenaManager();
      if (manager != null) {
        manager.computeScroll();
      }
    }

    @Override
    public boolean canScrollHorizontally(int direction) {
      UIList list = mWeakUIList.get();
      if (list != null && list.mEnableStrictScrollable) {
        // when the first/last component's height is 0, the
        // canScrollVertical()/canScrollHorizontally is not right.in order to solve the problem,we
        // can judge it by findFirstCompleteLyListItem or findLastCompleteLyListItem.return false
        // only on two sceens:
        // 1.  direction < 0 && findFirstCompleteLyListItem == 0
        // 2.  direction > 0 && findLastCompleteLyListItem() >= getAdapter().getItemCount() - 1
        if (direction < 0) {
          int pos = list.findFirstCompleteLyListItem();
          return pos == 0 ? false : super.canScrollHorizontally(direction);
        } else if (direction > 0) {
          int pos = list.findLastCompleteLyListItem();
          return pos >= getAdapter().getItemCount() - 1 ? false
                                                        : super.canScrollHorizontally(direction);
        }
      }
      return super.canScrollHorizontally(direction);
    }

    @Override
    public boolean canScrollVertically(int direction) {
      UIList list = mWeakUIList.get();
      if (list != null && list.mEnableStrictScrollable) {
        // when the first/last component's height is 0, the
        // canScrollVertical()/canScrollHorizontally is not right.in order to solve the problem,we
        // can judge it by findFirstCompleteLyListItem or findLastCompleteLyListItem.return false
        // only on two sceens:
        // 1.  direction < 0 && findFirstCompleteLyListItem == 0
        // 2.  direction > 0 && findLastCompleteLyListItem() >= getAdapter().getItemCount() - 1
        if (direction < 0) {
          int pos = list.findFirstCompleteLyListItem();
          return pos == 0 ? false : super.canScrollVertically(direction);
        } else if (direction > 0) {
          int pos = list.findLastCompleteLyListItem();
          if (getAdapter() != null) {
            return pos >= getAdapter().getItemCount() - 1 ? false
                                                          : super.canScrollVertically(direction);
          }
        }
      }
      return super.canScrollVertically(direction);
    }

    @Override
    public View focusSearch(View focused, int direction) {
      if (mWeakUIList != null && mWeakUIList.get() != null) {
        UIList uiList = mWeakUIList.get();
        if (!uiList.mEnableFocusSearch && (direction == FOCUS_DOWN || direction == FOCUS_UP)) {
          return focused;
        }
      }
      return super.focusSearch(focused, direction);
    }

    private void detectNestedScroll(boolean scroll) {
      if (mWeakContext == null || mWeakUIList == null) {
        return;
      }
      LynxContext context = mWeakContext.get();
      LynxBaseUI ui = mWeakUIList.get();
      if (scroll && context != null && ui != null) {
        context.onGestureRecognized(ui);
      }
    }

    @Override
    public boolean dispatchNestedPreScroll(
        int dx, int dy, int[] consumed, int[] offsetInWindow, int type) {
      boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
      detectNestedScroll(res);
      return res;
    }

    @Override
    public boolean dispatchNestedPreScroll(int dx, int dy, int[] consumed, int[] offsetInWindow) {
      boolean res = super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow);
      detectNestedScroll(res);
      return res;
    }

    public void setTouchScroll(boolean scroll) {
      mTouchScroll = scroll;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent e) {
      if (mPreferenceConsumeGesture) {
        requestDisallowInterceptTouchEvent(true);
      }
      UIList list = mWeakUIList.get();
      if (list == null) {
        return super.onInterceptTouchEvent(e);
      }
      if (list.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
          && e.getActionMasked() != MotionEvent.ACTION_DOWN) {
        // If new gestures are enabled, return false to indicate that the event is not intercept, So
        // this event can be passed to child node, do not intercept the down event, otherwise will
        // not receive other types of events.
        return false;
      }
      return super.onInterceptTouchEvent(e);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
      UIList list = mWeakUIList.get();
      if (list == null) {
        return super.dispatchTouchEvent(ev);
      }
      if (list.isEnableNewGesture()) {
        if (Boolean.FALSE.equals(mConsumeGesture)) {
          return true;
        }
        if (ev.getActionMasked() == MotionEvent.ACTION_MOVE) {
          if (mConsumeGesture != null && !mIsDownEventHandled) {
            ev.setAction(MotionEvent.ACTION_DOWN);
            mIsDownEventHandled = true;
          }
        }
      }
      return super.dispatchTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
      UIList list = mWeakUIList.get();
      if (list == null) {
        return super.onTouchEvent(ev);
      }
      if (list.isEnableNewGesture() && (mConsumeGesture == null || !mConsumeGesture)
          && ev.getActionMasked() != MotionEvent.ACTION_DOWN) {
        // If new gestures are enabled, return false to indicate that the event is not consumed,
        // So this event can be passed to parent node, do not intercept the down event, otherwise
        // will not receive other types of events.
        return false;
      }
      if (mTouchScroll) {
        return super.onTouchEvent(ev);
      }
      return true;
    }

    @Override
    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
      if (mWeakContext != null && mWeakContext.get() != null) {
        LynxContext context = mWeakContext.get();
        LynxAccessibilityWrapper wrapper = context.getLynxAccessibilityWrapper();
        if (wrapper != null && wrapper.enableDelegate()) {
          return super.requestChildRectangleOnScreen(child, rect, false);
        }
      }
      return super.requestChildRectangleOnScreen(child, rect, immediate);
    }

    public void consumeGesture(boolean consume) {
      mConsumeGesture = consume;
      if (consume) {
        mIsDownEventHandled = false;
      }
    }
  }

  protected RecyclerView createRecyclerView(Context context) {
    return new PrivateRecyclerView(context, this);
  }

  public RecyclerView getRecyclerView() {
    return this.getView();
  }

  protected void setAdapter(UIListAdapter adapter) {
    this.mAdapter = adapter;
  }

  protected UIListAdapter getAdapter() {
    return mAdapter;
  }

  protected void setAppearEventCourier(AppearEventCourier courier) {
    mAppearEventCourier = courier;
  }

  @Override
  public boolean isScrollContainer() {
    return true;
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (noInvalidate) {
      this.noInvalidate = false;
      return;
    }

    if (mView.getAdapter() == null) {
      mView.setAdapter(mAdapter);
    }

    if (mGestureHandlers != null) {
      GestureArenaManager manager = getGestureArenaManager();
      // Check if the current UIList instance is already a member of the gesture arena
      if (manager != null && !manager.isMemberExist(getGestureArenaMemberId())) {
        // If not a member, add the UIList instance as a new member to the gesture arena
        mGestureArenaMemberId = manager.addMember(UIList.this);
      }
    }

    if (!isNeedRenderComponents()) {
      reportException();
      return;
    }
    if (this.mNewArch) {
      mAdapter.isAsync = isAsyncThreadStrategy();
      // scroll to the target position
      if (mScrollIndex >= 0 && mScrollIndex < mAdapter.getItemCount()) {
        getView().scrollToPosition(mScrollIndex);
        mScrollIndex = -1;
      }

      if (mListNoDiffInfo != null) {
        mAdapter.updateListActionInfo(mListNoDiffInfo);
        mListNoDiffInfo = null;
      } else if (mListPlatformInfo instanceof ReadableMap) {
        if (mPreloadBufferCount > 0 && isAsyncThreadStrategy()) {
          if (mPreloadCache == null) {
            mPreloadCache = new ListPreloadCache(this, mPreloadBufferCount);
          }
          setPreBufferListener();
        }
        mAdapter.updateChildrenInfo((JavaOnlyMap) mListPlatformInfo);
        mListPlatformInfo = null;
      }
    } else {
      mAdapter.updateChildrenInfo(getPlatformInfo());
    }
    updateLayoutMangerIfNeeded();
    setReverseLayout(mEnableRTL && isRtl());
    int sizeCount = mAdapter.mViewNames == null ? 0 : mAdapter.mViewNames.size();
    if (sizeCount > mInitialScrollIndex && mInitialScrollIndex > RecyclerView.NO_POSITION) {
      mListScroller.scrollToPositionInner(mInitialScrollIndex);
      mInitialScrollIndex = RecyclerView.NO_POSITION;
    }
    LLog.i(TAG, "onPropsUpdated viewNames " + sizeCount);

    if (mListEventManager.isLayoutCompleteEnable()) {
      // Note: In fiber arch, the update action may not invoke RV's notifyItemChanged() if the
      // 'flush' value of update action is false, in this case, we should not set
      // mNeedLayoutComplete to true, otherwise the onLayoutCompleted callback will be invoked in
      // the next layout pass, which is a redundant callback
      mNeedLayoutComplete = !mFiberArch || mView.isLayoutRequested();
    }

    if (mListStickyManager != null) {
      // when data change, sticky component need be updated
      // sticky component control by StickyManager, not UIListAdapter
      mListStickyManager.flushStickyComponent();

      // fix the problem of props dependency
      mListStickyManager.setUseOldStickCategory(mPendingOldStickCategory);
    }
    mListScroller.setVerticalOrientation(isVertical());

    // sometimes the  lower/upper method  cannot be called, fix the problem by resetting last
    // scrolling status
    if (mUpperLowerSwitch && mListEventManager != null) {
      mListEventManager.resetScrollBorderStatus();
    }

    updateGapItemDecorationIfNeed();
  }

  /**
   * checks if list can render components on next step;
   * @return true, only on two situations
   *    *(1) the thread strategy is MOST_ON_TASM or MULTI_THREADS, the list is new architecture and
   * mEnableAsyncList is true;
   *    *(2) the thread strategy is not MOST_ON_TASM or MULTI_THREADS;
   */
  private boolean isNeedRenderComponents() {
    if (isAsyncThreadStrategy()) {
      return mNewArch ? mEnableAsyncList : false;
    }
    return true;
  }

  private void reportException() {
    if (mContext == null || mContext.getLynxView() == null) {
      return;
    }
    LynxError error = new LynxError(LynxSubErrorCode.E_COMPONENT_LIST_UNSUPPORTED_THREAD_STRATEGY,
        "Multi thread strategy can not be used by default.",
        "Please set the attribute of enable-async-list to true at LynxSDK 2.10+ .",
        LynxError.LEVEL_ERROR);
    mContext.handleLynxError(error);
  }

  @Override
  public void onLayoutFinish(long operationId, LynxBaseUI component) {
    if (isAsyncThreadStrategy()) {
      if (component instanceof UIComponent) {
        mAdapter.onLayoutFinishAsync((UIComponent) component, operationId);
      } else {
        LLog.e(TAG, "component is null! the operationId is " + operationId);
      }
    } else {
      mAdapter.onLayoutFinish(operationId);
    }
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    // set paddingLeft, paddingRight into recycler view
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;

    if (LAYOUT_DIRECTION_RTL == mView.getLayoutDirection()) {
      mView.setPadding(paddingRight, paddingTop, paddingLeft, paddingBottom);
    } else {
      mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom);
    }
  }

  @Override
  public void onInsertChild(LynxBaseUI child, int index) {
    if (DEBUG) {
      LLog.i(TAG, "insertChild index: " + index + " child: " + child);
    }
  }

  public boolean isAsyncThreadStrategy() {
    if (mContext == null || mContext.getLynxView() == null) {
      return false;
    }

    if (mContext.getEnableAutoConcurrency()) {
      return mEnableAsyncList;
    }

    ThreadStrategyForRendering strategy = mContext.getLynxView().getThreadStrategyForRendering();
    return strategy == ThreadStrategyForRendering.MOST_ON_TASM
        || strategy == ThreadStrategyForRendering.MULTI_THREADS;
  }

  @Override
  public void measure() {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = TAG + ".measure";
      TraceEvent.beginSection(traceEvent);
    }
    boolean isLayoutRequested =
        mContainerView != null ? mContainerView.isLayoutRequested() : mView.isLayoutRequested();
    if (!isLayoutRequested) {
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
      return;
    }
    measureChildren();
    // When RefreshLayout contains UIList and UIList has sticky components, a container view
    // containing UIList and sticky components will be added to RefreshLayout instead of UIList.
    // In this case, UIList(recyclerView)'s layoutParams is the instance of
    // RelativeLayout#LayoutParams instead of FrameLayout#LayoutParams, and crash will happen in
    // mContainerView.measure().
    setLayoutParamsInternal();
    int widthSpec = View.MeasureSpec.makeMeasureSpec(getWidth(), View.MeasureSpec.EXACTLY);
    int heightSpec;
    if (mAutoMeasure) {
      if (DEBUG) {
        LLog.i(TAG, "UIList autoMeasure maxHeight " + mMaxHeight);
      }
      heightSpec = View.MeasureSpec.makeMeasureSpec((int) mMaxHeight, View.MeasureSpec.AT_MOST);
    } else {
      heightSpec = View.MeasureSpec.makeMeasureSpec(getHeight(), View.MeasureSpec.EXACTLY);
    }

    if (mContainerView != null) {
      mContainerView.measure(widthSpec, heightSpec);
    }
    // The old measure logic: if mContainerView is not null, we only measure mContainerView and if
    // mContainerView is null, we only measure RecyclerView. In certain scenarios, such as
    // RecyclerView has not been added to mContainerView when UIList#measure() is invoked, the old
    // logic may result in RecyclerView not measured.
    // So we invoke RecyclerView#measure() once actively to make sure UIList's width and height are
    // set to the RecyclerView.
    mView.measure(widthSpec, heightSpec);
    mAdapter.mDiffResultConsumed = true;
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @Override
  public void layout() {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = TAG + ".layout";
      TraceEvent.beginSection(traceEvent);
    }
    boolean isLayoutRequested =
        mContainerView != null ? mContainerView.isLayoutRequested() : mView.isLayoutRequested();
    if (!isLayoutRequested) {
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
      return;
    }
    layoutChildren();
    if (mContainerView != null) {
      mContainerView.layout(getLeft(), getTop(), getLeft() + getWidth(), getTop() + getHeight());
    } else {
      mView.layout(getLeft(), getTop(), getLeft() + getWidth(), getTop() + getHeight());
    }
    setClipChildrenInternal();
    ViewCompat.setClipBounds(mView, getBoundRectForOverflow());
    mAppearEventCourier.onListLayout();
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  private void setClipChildrenInternal() {
    if (!mEnableOverflow) {
      return;
    }
    ViewParent parentView = mContainerView != null ? mContainerView.getParent() : mView.getParent();
    boolean isOverflow = getOverflow() != 0x00;
    if (isOverflow && parentView instanceof ViewGroup) {
      ViewGroup viewGroup = (ViewGroup) parentView;
      viewGroup.setClipChildren(false);
    }
    // Note: if the List has mContainerView, both the mContainerView and its parent view should be
    // set clip children to false.
    if (isOverflow && mContainerView != null) {
      mContainerView.setClipChildren(false);
    }
    // Note: If list is not overflow hidden, we should close RecyclerView over scroll otherwise
    // RecyclerView will clip draw area to its bound when drag the RecyclerView.
    if (mOverflow != OVERFLOW_HIDDEN) {
      mView.setOverScrollMode(OVER_SCROLL_NEVER);
    }
  }

  @Override
  protected Rect getBoundRectForOverflow() {
    if (!mEnableOverflow) {
      return super.getBoundRectForOverflow();
    } else {
      return super.getClipBounds();
    }
  }

  @Override
  public void requestLayout() {
    /* if flag  PFLAG_FORCE_LAYOUT is set, nothing to do
     *  need check result of requestLayout(), we find requestLayout() fail when recyclerview is
     * layout. if not success, try on main loop again
     * */
    mView.requestLayout();
    if (!mView.isLayoutRequested()) {
      final View view = mView;
      mView.post(new Runnable() {
        @Override
        public void run() {
          view.requestLayout();
        }
      });
    }
  }

  @Override
  public void layoutChildren() {
    if (mTriggerStickyLayout && mContainerView != null) {
      UIComponent stickTopComponent = mListStickyManager.getStickyTopComponent();
      if (stickTopComponent != null) {
        stickTopComponent.performLayoutChildrenUI();
      }
      UIComponent stickBottomComponent = mListStickyManager.getStickyBottomComponent();
      if (stickBottomComponent != null) {
        stickBottomComponent.performLayoutChildrenUI();
      }
    }
    super.layoutChildren();
  }

  @Override
  protected void consumeGesture(boolean consumeGesture) {
    if (mView instanceof PrivateRecyclerView) {
      ((PrivateRecyclerView) mView).consumeGesture(consumeGesture);
    }
  }

  @MainThread
  @Override
  public float[] scrollBy(float deltaX, float deltaY) {
    float[] res = new float[4];
    mView.scrollBy((int) deltaX, (int) deltaY);
    // when scroll, not trigger basic events
    if (Math.abs(deltaX) > Float.MIN_VALUE || Math.abs(deltaY) > Float.MIN_VALUE) {
      recognizeGesturere();
    }
    if (mView.getLayoutManager() instanceof ListLayoutManager.ListLayoutInfo) {
      if (isVertical()) {
        res[0] = 0;
        res[1] = ((ListLayoutManager.ListLayoutInfo) mView.getLayoutManager()).getConsumedY();
        res[2] = deltaX;
        res[3] = deltaY - res[1];
      } else {
        res[0] = ((ListLayoutManager.ListLayoutInfo) mView.getLayoutManager()).getConsumedX();
        res[1] = 0;
        res[2] = deltaX - res[0];
        res[3] = deltaY;
      }
    } else {
      res[0] = 0;
      res[1] = 0;
      res[2] = deltaX;
      res[3] = deltaY;
    }

    return res;
  }

  @LynxProp(name = "enable-new-exposure-strategy", defaultBoolean = false)
  public void setNewAppear(boolean value) {
    mAppearEventCourier.setNewAppear(value);
  }

  @Override
  public void setScrollY(Dynamic enable) {}

  @Override
  public void setScrollX(Dynamic enable) {}

  @Override
  public void setUpperThreshold(Dynamic value) {
    mListEventManager.setUpperThreshold(value);
  }

  @Override
  public void setLowerThreshold(Dynamic value) {
    mListEventManager.setLowerThreshold(value);
  }

  @Override
  public void setUpperThresholdItemCount(Dynamic value) {
    mListEventManager.setUpperThresholdItemCount(value);
  }

  @Override
  public void setLowerThresholdItemCount(Dynamic value) {
    mListEventManager.setLowerThresholdItemCount(value);
  }

  @Override
  public void setScrollEventThrottle(Dynamic value) {
    mListEventManager.setScrollEventThrottle(value);
  }

  @Override
  public void setScrollStateChangeEventThrottle(String value) {}

  @Override
  public void setCacheQueueRatio(Dynamic value) {}

  @Override
  public void setColumnCount(int columnCount) {
    if (mColumnCount == columnCount) {
      return;
    }

    mColumnCount = columnCount;
    RecyclerView.LayoutManager lm = mView.getLayoutManager();
    if (lm instanceof GridLayoutManager) {
      ((GridLayoutManager) lm).setSpanCount(mColumnCount);
    } else if (lm instanceof StaggeredGridLayoutManager) {
      ((StaggeredGridLayoutManager) lm).setSpanCount(mColumnCount);
    }
  }

  @Override
  public void setMainAxisGap(float value) {
    mMainAxisGap = Math.round(value);
  }

  @Override
  public void setCrossAxisGap(float gap) {
    mCrossAxisGap = Math.round(gap);
    RecyclerView.LayoutManager lm = mView.getLayoutManager();
    if (lm instanceof ListLayoutManager.ListGridLayoutManager) {
      ((ListLayoutManager.ListGridLayoutManager) lm).setCrossAxisGap(getCrossAxisGap());
    } else if (lm instanceof ListLayoutManager.ListStaggeredGridLayoutManager) {
      ((ListLayoutManager.ListStaggeredGridLayoutManager) lm).setCrossAxisGap(getCrossAxisGap());
    }
  }

  public int getMainAxisGap() {
    return mEnableGapItemDecoration ? 0 : mMainAxisGap;
  }

  public int getCrossAxisGap() {
    return mEnableGapItemDecoration ? 0 : mCrossAxisGap;
  }

  @Override
  public void setListType(String listType) {
    if (TextUtils.isEmpty(listType)) {
      listType = LIST_TYPE_SINGLE;
    }
    if (!TextUtils.equals(listType, mListType)) {
      mNeedUpdateLayoutManager = true;
      mListType = listType;
    }
  }

  @Override
  public void setUpdateAnimation(String animationType) {
    if (TextUtils.isEmpty(animationType) || TextUtils.equals(animationType, "none")) {
      mView.setItemAnimator(null);
    }

    if (TextUtils.equals(animationType, "default")) {
      mView.setItemAnimator(new DefaultItemAnimator());
    }
  }

  @Override
  public void setEnablePagerSnap(Dynamic enable) {
    boolean pagingEnable = ListEventManager.dynamicToBoolean(enable, false);
    if (pagingEnable && mFactorSnapHelper == null) {
      if (mSnapHelper == null) {
        mSnapHelper = new PagerSnapHelper();
      }
      mSnapHelper.attachToRecyclerView(mView);
    } else {
      if (mSnapHelper != null) {
        mSnapHelper.attachToRecyclerView(null);
        mSnapHelper = null;
      }
    }
  }

  @Override
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
        if (mFactorSnapHelper == null) {
          mFactorSnapHelper = new FactoredPagerSnapHelper();
          mFactorSnapHelper.attachToRecyclerView(mView);
          mFactorSnapHelper.mPagerHooks = new FactoredPagerSnapHelper.FactoredPagerHooks() {
            @Override
            public void willSnapTo(int position, int currentOffsetX, int currentOffsetY,
                int distanceX, int distanceY) {
              UIList.this.willSnapTo(
                  position, currentOffsetX, currentOffsetY, distanceX, distanceY);
            }
          };
        }
        mFactorSnapHelper.setPagerAlignFactor(factor);
        mFactorSnapHelper.setPagerAlignOffset(offset);
        return;
      }
    }
    if (mFactorSnapHelper != null) {
      mFactorSnapHelper.attachToRecyclerView(null);
      mFactorSnapHelper = null;
    }
  }

  public void willSnapTo(
      int position, int currentOffsetX, int currentOffsetY, int distanceX, int distanceY) {
    currentOffsetX = mVerticalOrientation ? currentOffsetX : mListEventManager.getScrollOffset();
    currentOffsetY = mVerticalOrientation ? mListEventManager.getScrollOffset() : currentOffsetY;
    int targetOffsetX = currentOffsetX + distanceX;
    int targetOffsetY = currentOffsetY + distanceY;
    if (currentOffsetX != targetOffsetX || currentOffsetY != targetOffsetY) {
      LynxDetailEvent event = new LynxDetailEvent(getSign(), "snap");
      event.addDetail("position", position);
      event.addDetail("currentScrollLeft", PixelUtils.pxToDip(currentOffsetX));
      event.addDetail("currentScrollTop", PixelUtils.pxToDip(currentOffsetY));
      event.addDetail("targetScrollLeft", PixelUtils.pxToDip(targetOffsetX));
      event.addDetail("targetScrollTop", PixelUtils.pxToDip(targetOffsetY));
      mContext.getEventEmitter().sendCustomEvent(event);
    }
  }

  @Override
  public void setEnableSticky(Dynamic enable) {
    boolean sticky = ListEventManager.dynamicToBoolean(enable, false);
    if (sticky && mContainerView == null) {
      mListStickyManager = new ListStickyManager(this);
      mContainerView = mListStickyManager.getContainer();
      mListStickyManager.setStickyOffset(mPendingStickyOffset);
    }
  }

  @Override
  public void setStickyOffset(Dynamic value) {
    int offset = ListEventManager.dynamicToInt(value, 0);
    offset = (int) PixelUtils.dipToPx(offset);
    if (mListStickyManager == null) {
      mPendingStickyOffset = offset;
    } else {
      mListStickyManager.setStickyOffset(offset);
    }
  }

  public View getContainer() {
    return mContainerView;
  }

  @Override
  public void sendCustomEvent(int left, int top, int dx, int dy, String type) {}

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    if (mAdapter == null) {
      return this;
    }

    EventTarget target = mListStickyManager != null
        ? mListStickyManager.hitTest((int) x, (int) y, ignoreUserInteraction)
        : null;
    if (target != null) {
      return target;
    }

    for (int i = mView.getChildCount() - 1; i >= 0; i--) {
      View childView = mView.getChildAt(i);
      RecyclerView.ViewHolder childViewHolder = mView.getChildViewHolder(childView);
      if (childViewHolder instanceof ListViewHolder) {
        ListViewHolder holder = (ListViewHolder) childViewHolder;
        if (holder != null && holder.getUIComponent() != null) {
          UIComponent ui = holder.getUIComponent();
          boolean contain = ui.containsPoint(
              x - childView.getLeft(), y - childView.getTop(), ignoreUserInteraction);
          if (contain) {
            return ui.hitTest(
                x - childView.getLeft(), y - childView.getTop(), ignoreUserInteraction);
          }
        }
      }
    }

    return this;
  }

  void onLayoutCompleted() {
    LLog.i(UIList.TAG, "onLayoutCompleted " + mAdapter.mViewNames.size());
    if (mNeedLayoutComplete && mView.getChildCount() > 0) {
      mListEventManager.sendLayoutCompleteEvent(mAdapter.mViewNames);
      mNeedLayoutComplete = false;
    }
  }

  private void updateLayoutMangerIfNeeded() {
    if (mNeedUpdateLayoutManager) {
      getAdapter().initItemHeightData();
      RecyclerView.LayoutManager layoutManager = null;
      final WeakReference<UIList> listRef = new WeakReference<>(this);
      if (TextUtils.equals(mListType, LIST_TYPE_SINGLE)) {
        layoutManager = new ListLayoutManager.ListLinearLayoutManager(mContext, this);

        ((ListLayoutManager.ListLinearLayoutManager) layoutManager)
            .setOrientation(mVerticalOrientation ? LinearLayoutManager.VERTICAL
                                                 : LinearLayoutManager.HORIZONTAL);
        if (!mEnableItemPrefetch) {
          ((ListLayoutManager.ListLinearLayoutManager) layoutManager)
              .setInitialPrefetchItemCount(0);
        }
      } else if (TextUtils.equals(mListType, LIST_TYPE_FLOW)) {
        layoutManager = new ListLayoutManager.ListGridLayoutManager(
            mContext, mColumnCount, getCrossAxisGap(), this);
        ((ListLayoutManager.ListGridLayoutManager) layoutManager)
            .setOrientation(mVerticalOrientation ? LinearLayoutManager.VERTICAL
                                                 : LinearLayoutManager.HORIZONTAL);
        if (!mEnableItemPrefetch) {
          ((ListLayoutManager.ListGridLayoutManager) layoutManager).setInitialPrefetchItemCount(0);
        }
      } else if (TextUtils.equals(mListType, LIST_TYPE_WATERFALL)) {
        layoutManager = new ListLayoutManager.ListStaggeredGridLayoutManager(
            mColumnCount, getCrossAxisGap(), StaggeredGridLayoutManager.VERTICAL, this);
        ((ListLayoutManager.ListStaggeredGridLayoutManager) layoutManager)
            .setOrientation(mVerticalOrientation ? StaggeredGridLayoutManager.VERTICAL
                                                 : StaggeredGridLayoutManager.HORIZONTAL);
      }
      if (mListStickyManager != null) {
        mListStickyManager.clear();
      }
      if (!mEnableItemPrefetch && layoutManager != null) {
        layoutManager.setItemPrefetchEnabled(false);
      }
      // close itemView cache size,use ListPrelaodCache
      if (mPreloadCache != null) {
        mView.setItemViewCacheSize(0);
      }
      mView.setLayoutManager(layoutManager);
    }
    mNeedUpdateLayoutManager = false;

    if (mView.getLayoutManager() instanceof ListLayoutManager.ListLinearLayoutManager) {
      ListLayoutManager.ListLinearLayoutManager listLinearLayoutManager =
          ((ListLayoutManager.ListLinearLayoutManager) mView.getLayoutManager());
      if (listLinearLayoutManager.getStackFromEnd() != mStackFromEnd) {
        listLinearLayoutManager.setStackFromEnd(mStackFromEnd);
      }
    }

    if (mView.getLayoutManager() instanceof GridLayoutManager) {
      final GridLayoutManager layoutManager = (GridLayoutManager) mView.getLayoutManager();
      layoutManager.setSpanSizeLookup(new GridLayoutManager.SpanSizeLookup() {
        @Override
        public int getSpanSize(int position) {
          if (mAdapter.isFullSpan(position) && mColumnCount > 1) {
            return layoutManager.getSpanCount();
          }
          return 1;
        }
      });
    }
  }

  /**
   * Sets whether LayoutManager should start laying out items from the end of the UI.
   */
  private void setReverseLayout(boolean reverseLayout) {
    mView.setLayoutDirection(reverseLayout ? LAYOUT_DIRECTION_RTL : LAYOUT_DIRECTION_LTR);
  }

  @LynxUIMethod
  public void autoScroll(ReadableMap params, Callback callback) {
    String ratePerSecond = params.getString("rate", "");
    // if mStart is true, should start autoScroll; otherwise, stop scrolling
    mStart = params.getBoolean("start", true);
    mAutoStopOnBounds = params.getBoolean("autoStop", true);
    if (mStart) {
      int px =
          (int) UnitUtils.toPxWithDisplayMetrics(ratePerSecond, 0, 0, mContext.getScreenMetrics());
      if (px == 0) {
        callback.invoke(LynxUIMethodConstants.UNKNOWN, "rate is not right");
        return;
      }
      int refreshRate = (int) DeviceUtils.getRefreshRate(getLynxContext());
      // prevent 0 from being a divisor
      if (refreshRate <= 0) {
        refreshRate = DeviceUtils.DEFAULT_DEVICE_REFRESH_RATE;
      }
      // if list's scrolling direction is up, mAutoRatePerFrame >0; otherwise,mAutoRatePerFrame <0;
      mAutoRatePerFrame = px > 0 ? Math.max(px / refreshRate, 1) : Math.min(px / refreshRate, -1);
      // stop last scroll by removing FrameCallback
      removeFrameCallback();
      autoScroll();
    } else {
      removeFrameCallback();
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS);
  }

  // According to the layout direction, determine whether it is possible to scroll
  // Negative to check scrolling up/left, positive to check scrolling down/right.
  boolean canScroll(int direction) {
    if (mVerticalOrientation) {
      return getView().canScrollVertically(direction);
    } else {
      return getView().canScrollHorizontally(direction);
    }
  }

  void autoScroll() {
    mFrameCallback = new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        // Check if this view can be scrolled vertically/horizontally in a certain direction.
        // direction:Negative to check scrolling up, positive to check scrolling down.
        boolean canScroll =
            ((mAutoRatePerFrame > 0 && canScroll(1)) || (mAutoRatePerFrame < 0 && canScroll(-1)));
        if (canScroll) {
          if (mVerticalOrientation) {
            getView().scrollBy(0, mAutoRatePerFrame);
          } else {
            getView().scrollBy(mAutoRatePerFrame, 0);
          }
        }
        // should post FrameCallback on two scenes:
        // firstly: the switch of "enableAutoScroll" is true and the list  can be Scrolled.
        // secondly:the switch of "enableAutoScroll" is true and mAutoStopOnBounds is
        // false.mAutoStopOnBounds is false, it means that the list can not stop auto scrolling even
        // if it comes to the boarder
        if (mStart && (canScroll || !mAutoStopOnBounds)) {
          if (mFrameCallback != null) {
            Choreographer.getInstance().postFrameCallback(mFrameCallback);
          }
        } else {
          // remove FrameCallback
          removeFrameCallback();
        }
      }
    };
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }

  public void scrollToPosition(ReadableMap params) {
    scrollToPosition(params, new Callback() {
      public void invoke(Object... args) {}
    });
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

  @LynxUIMethod
  public void scrollToPosition(ReadableMap params, Callback callback) {
    if (mAdapter == null) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "scrollToPosition before init");
      return;
    }
    int position = params.getInt(METHOD_PARAMS_INDEX, params.getInt(METHOD_PARAMS_POSITION, 0));
    double offset = params.getDouble(METHOD_PARAMS_OFFSET, 0.0);
    int offsetVal = (int) PixelUtils.dipToPx(offset);
    boolean smooth = params.getBoolean(METHOD_PARAMS_SMOOTH, false);
    double itemHeight = params.getDouble(METHOD_PARAMS_ITEM_HEIGHT, 0.0);
    int heightVal = (int) PixelUtils.dipToPx(itemHeight);
    String alignTo = params.getString(METHOD_PARAMS_ALIGN_TO, ALIGN_TO_NONE);

    if (position < 0 || position > mAdapter.getItemCount()) {
      callback.invoke(
          LynxUIMethodConstants.PARAM_INVALID, "position < 0 or position >= data count");
      return;
    }
    if (smooth) {
      mListScroller.scrollToPositionSmoothly(position, alignTo, offsetVal, callback);
      return;
    }
    // It's hard to scroll no smoothly to specific position when enable async layout,
    // Due to that the height is not correct when scroll.
    // We need to calculate the height first and the start scroll.
    if (TextUtils.equals(alignTo, UIList.ALIGN_TO_MIDDLE)) {
      offsetVal += (getView().getHeight() - heightVal) / 2;
    } else if (TextUtils.equals(alignTo, UIList.ALIGN_TO_BOTTOM)) {
      offsetVal += getView().getHeight() - heightVal;
    }
    mListScroller.scrollToPositionInner(position, offsetVal, callback);

    mView.post(new Runnable() {
      @Override
      public void run() {
        // guarantee to flush the sticky Component after Scrolling
        if (mListStickyManager != null) {
          mListStickyManager.flushStickyComponentAfterScrolling();
        }
        // sendScrollEvent even if scroll non-smooth
        if (mNewScrollTop) {
          mListEventManager.mScrollTop = mAdapter.getScrollY();
          mListEventManager.sendScrollEvent(LynxListEvent.EVENT_SCROLL,
              ListEventManager.SCROLL_EVENT_ON, mListEventManager.mScrollTop,
              mListEventManager.mScrollTop, 0, 0);
        }
      }
    });
  }

  @LynxUIMethod
  public void getVisibleCells(Callback callback) {
    if (callback == null) {
      LLog.i(UIList.TAG, "getVisibleCells with null callback");
      return;
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS, mListEventManager.getVisibleCellsInfo());
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    mListEventManager.setEvents(events);
  }

  /** ---------- Accessibility Section ---------- */

  @Override
  public boolean isAccessibilityHostUI() {
    return true;
  }

  @Override
  public boolean isAccessibilityDirectionVertical() {
    return mVerticalOrientation;
  }

  /**
   * Request that ui can be visible on the screen and scrolling if necessary just enough.
   *
   * @param child the direct ui making the request.
   * @param rect the rectangle in the child's coordinates the child wishes to be on the screen.
   * @param smooth animated scrolling.
   * @return true if list scrolled to handle the operation
   */
  @Override
  public boolean requestChildUIRectangleOnScreen(LynxBaseUI child, Rect rect, boolean smooth) {
    if (!mEnableScroll || !(child instanceof UIComponent)
        || ((UIComponent) child).getView() == null) {
      return false;
    }
    ViewParent componentWrapView = ((UIComponent) child).getView().getParent();
    if (!(componentWrapView instanceof ListViewHolder.WrapView)) {
      return false;
    }
    View wrapView = (View) componentWrapView;
    return mView.getLayoutManager().requestChildRectangleOnScreen(
        mView, wrapView, rect, false, false);
  }

  public ArrayList<String> getComponentAccessibilityOrder() {
    return mComponentAccessibilityOrder;
  }

  public HashMap<String, ArrayList<LynxNodeProvider.LynxCustomNodeInfo>> getCustomNodeMap() {
    return mCustomNodeMap;
  }

  public void initNodeInfo() {
    for (ArrayList<LynxNodeProvider.LynxCustomNodeInfo> nodeInfoList : mCustomNodeMap.values()) {
      for (LynxNodeProvider.LynxCustomNodeInfo nodeInfo : nodeInfoList) {
        nodeInfo.invalid();
      }
    }
  }

  public void updateNodeInfo(
      final String itemKey, final ArrayList<LynxNodeProvider.LynxCustomNodeInfo> nodeInfoList) {
    if (!mCustomNodeMap.containsKey(itemKey)) {
      mComponentAccessibilityOrder.add(itemKey);
    }
    mCustomNodeMap.put(itemKey, nodeInfoList);
    if (LynxAccessibilityDelegate.DEBUG) {
      LLog.i("LynxA11yList", "updateNodeInfo: " + itemKey + ", map size: " + mCustomNodeMap.size());
    }
  }

  /** ---------- Accessibility Section end ---------- */

  @LynxProp(name = "vertical-orientation", defaultBoolean = true)
  public void setVerticalOrientation(boolean verticalOrientation) {
    mVerticalOrientation = verticalOrientation;
  }

  @LynxProp(name = "auto-measure", customType = "false")
  public void setAutoMeasure(Dynamic enable) {
    mAutoMeasure = ListEventManager.dynamicToBoolean(enable, false);
  }

  @LynxProp(name = "android-diffable", customType = "true")
  public void setDiffable(Dynamic value) {
    if (mView.getAdapter() == null) {
      boolean diffable = ListEventManager.dynamicToBoolean(value, true);
      mAdapter.setHasStableIds(!diffable);
    }
  }
  @LynxProp(name = "use-old-sticky", defaultBoolean = true)
  public void setListOldStickySwitch(boolean value) {
    mPendingOldStickCategory = value;
  }

  /**
   * @name: android-new-scroll-top
   * @description: Uses height accumulation method to calculate scrollTop switch
   * @category: temporary
   * @standardAction: offline
   * @supportVersion: 2.8
   **/
  @LynxProp(name = "android-new-scroll-top", defaultBoolean = false)
  public void setListNewScrollTopSwitch(boolean value) {
    mNewScrollTop = value;
  }

  @LynxProp(name = "scroll-upper-lower-switch", defaultBoolean = false)
  public void setUpperLowerSwitch(boolean value) {
    mUpperLowerSwitch = value;
  }

  /**
   * @name: scroll-index
   * @description: Every time it is set, it will take effect, it replaces the "initial-scroll-index"
   *to keep consistency on different platforms.
   * @standardAction: keep
   * @supportVersion: 2.14
   **/
  @LynxProp(name = "scroll-index", defaultInt = -1)
  public void setScrollIndex(int value) {
    mScrollIndex = value;
  }

  @LynxProp(name = "android-enable-overflow", defaultBoolean = false)
  public void setEnableOverflow(boolean value) {
    mEnableOverflow = value;
    if (mView instanceof PrivateRecyclerView) {
      ((PrivateRecyclerView) mView).setEnableOverflow(value);
    }
  }

  @LynxProp(name = "list-platform-info")
  public void setListPlatformInfo(@NonNull ReadableMap map) {
    this.mNewArch = true;
    this.mListPlatformInfo = map;
    this.mListNoDiffInfo = null;
    this.mFiberArch = false;
  }

  @LynxProp(name = "update-list-info")
  public void updateListActionInfo(@NonNull ReadableMap map) {
    this.mNewArch = true;
    mListNoDiffInfo = map;
    this.mListPlatformInfo = null;
    this.mFiberArch = true;
  }

  /**
   * @name: android-preference-consume-gesture
   * @description: Request parent not intercept motion event for consuming.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.9
   **/
  @LynxProp(name = "android-preference-consume-gesture", defaultBoolean = false)
  public void setPreferenceConsumeGesture(boolean value) {
    if (mView instanceof PrivateRecyclerView) {
      ((PrivateRecyclerView) mView).mPreferenceConsumeGesture = value;
    }
  }

  /**
   * @name: android-trigger-sticky-layout
   * @description: In sticky cases, if the sticky component created by sticky manager and added to
   *ContainerView instead of removed from RecyclerView to ContainerView, in the next layout pass,
   *the sticky component may not invoke layoutChildren() to layout children, so we can set this
   *switch to true and trigger sticky component layout children in UIList#layoutChildren(). Note:
   *This is a temporary switch and it's default value will be changed to true in the feature.
   * @category: temporary
   * @standardAction: offline
   * @supportVersion: 2.8
   * @resolveVersion: 2.10
   **/
  @LynxProp(name = "android-trigger-sticky-layout", defaultBoolean = false)
  public void setTriggerStickyLayout(boolean value) {
    mTriggerStickyLayout = value;
  }

  /**
   * @name: android-stack-from-end
   * @description: The list fills its content starting from the bottom of the view,it only support
   *"single" list.
   * @platform: Android
   * @category: temporary
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = "android-stack-from-end", defaultBoolean = false)
  public void setStackFromEnd(boolean value) {
    mStackFromEnd = value;
  }

  @Override
  public void setInternalCellAppearNotification(boolean isNeedAppearNotification) {
    super.setInternalCellAppearNotification(isNeedAppearNotification);
    if (mAdapter != null) {
      mAdapter.setInternalCellAppearNotification(isNeedAppearNotification);
    }
  }

  @Override
  public void setInternalCellDisappearNotification(boolean isNeedDisAppearNotification) {
    super.setInternalCellDisappearNotification(isNeedDisAppearNotification);
    if (mAdapter != null) {
      mAdapter.setInternalCellDisappearNotification(isNeedDisAppearNotification);
    }
  }

  @Override
  public void setInternalCellPrepareForReuseNotification(boolean isNeedReuseNotification) {
    super.setInternalCellPrepareForReuseNotification(isNeedReuseNotification);
    if (mAdapter != null) {
      mAdapter.setInternalCellPrepareForReuseNotification(isNeedReuseNotification);
    }
  }

  public void setShouldRequestStateRestore(boolean shouldRequestStateRestore) {
    super.setInternalCellAppearNotification(shouldRequestStateRestore);
    if (mAdapter != null) {
      mAdapter.setInternalCellAppearNotification(shouldRequestStateRestore);
      mAdapter.setInternalCellDisappearNotification(shouldRequestStateRestore);
      mAdapter.setInternalCellPrepareForReuseNotification(shouldRequestStateRestore);
    }
  }

  public void setNeedVisibleCells(boolean needVisibleCells) {
    mListEventManager.mNeedsVisibleCells = needVisibleCells;
  }

  public void setScrollEnable(Dynamic value) {
    mEnableScroll = ListEventManager.dynamicToBoolean(value, true);
  }

  public void setTouchScroll(Dynamic value) {
    if (mView instanceof PrivateRecyclerView) {
      ((PrivateRecyclerView) mView).setTouchScroll(ListEventManager.dynamicToBoolean(value, true));
    }
  }

  @Override
  public void setComponentInitMeasure(boolean value) {
    if (mAdapter != null) {
      mAdapter.mComponentInitMeasure = value;
    }
  }

  @Override
  public void setNoInvalidate(boolean noInvalidate) {
    this.noInvalidate = noInvalidate;
  }

  @Deprecated
  public void setInitialRows(Dynamic initRows) {}

  public void setInitialScrollIndex(Dynamic value) {
    mInitialScrollIndex = ListEventManager.dynamicToInt(value, RecyclerView.NO_POSITION);
  }

  @Override
  public void destroy() {
    if (mPreloadListener != null) {
      getView().removeOnScrollListener(mPreloadListener);
    }
    if (mPreloadCache != null) {
      mPreloadCache.destroy();
    }

    super.destroy();
    // when list is destroyed, in order to avoid memory leak, should remove FrameCallback
    removeFrameCallback();
    nativeListStateCache.clear();
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

  @Override
  public void removeKeyFromNativeStorage(String key) {
    if (nativeListStateCache != null) {
      nativeListStateCache.remove(key);
    }
  }

  public Object getValueFromNativeStorage(String key) {
    if (nativeListStateCache != null) {
      return nativeListStateCache.get(key);
    }
    return null;
  }

  @Override
  public void storeKeyToNativeStorage(String key, Object value) {
    if (nativeListStateCache != null) {
      nativeListStateCache.put(key, value);
    }
  }

  public boolean initialPropsFlushed(String initialPropKey, String cacheKey) {
    if (nativeListStateCache != null) {
      HashSet initialPropFlushSet = initialFlushPropCache.get(initialPropKey);
      if (initialPropFlushSet != null && initialPropFlushSet.contains(initialPropKey)) {
        return true;
      }
    }
    return false;
  }

  public void setInitialPropsHasFlushed(String initialPropKey, String cacheKey) {
    if (nativeListStateCache != null) {
      HashSet initialPropFlushSet = initialFlushPropCache.getOrDefault(cacheKey, new HashSet<>());
      initialPropFlushSet.add(initialPropKey);
      nativeListStateCache.put(cacheKey, initialPropFlushSet);
    }
  }

  /**
   * Returns the horizontal scroll offset of the member.
   * @return The horizontal scroll offset.
   */
  @Override
  public int getMemberScrollX() {
    if (isVertical()) {
      return 0;
    }
    return mListEventManager.getScrollOffset();
  }

  /**
   * Returns the vertical scroll offset of the member.
   * @return The vertical scroll offset.
   */
  @Override
  public int getMemberScrollY() {
    if (isVertical()) {
      return mListEventManager.getScrollOffset();
    } else {
      return 0;
    }
  }

  /**
   * is the list's orientation  vertical
   *
   * @return
   */
  public boolean isVertical() {
    return mVerticalOrientation;
  }

  boolean getUpperLowerSwitch() {
    return mUpperLowerSwitch;
  }

  /**
   * @name: android-ignore-attach-for-binding
   * @description: Ignore the AttachToWindow switch and bind data to the corresponding position.
   * @category: temporary
   * @standardAction: offline
   * @supportVersion: 2.9
   **/
  @LynxProp(name = "android-ignore-attach-for-binding", defaultBoolean = false)
  public void setAndroidIgnoreAttachForBinding(boolean value) {
    mIgnoreAttachForBinding = value;
  }

  /**
   * @name: android-enable-item-prefetch
   * @description: Sets whether the LayoutManager of RecyclerView should be queried for views
   *outside its viewport while the UI thread is idle between frames. If enabled, the
   *LayoutManager will be queried for items to inflate/bind in between view system traversals. The
   *default value is true and default and the number of items to prefetch when first coming on
   *screen with new data is 2.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.11
   **/
  @LynxProp(name = "android-enable-item-prefetch", defaultBoolean = true)
  public void setEnableItemPrefetch(boolean value) {
    mEnableItemPrefetch = value;
  }

  /**
   * @name: enable-async-list
   * @description: enable create node async on List
   * @category: temporary
   * @standardAction: offline
   * @supportVersion: 2.10
   **/
  @LynxProp(name = "enable-async-list", defaultBoolean = false)
  public void setEnableAsyncList(boolean value) {
    mEnableAsyncList = value;
  }

  /**
   * @name: item-holder-type
   * @description: on the multi-thread strategy, when the component A reuse the view of the
   * component B. there are different modes to show the component‘s placeHolder before the view of
   * component A  has the right result. 1.just do nothing, use the B's view as the place holder. 2.
   * remove the item holder's view under any circumstances。 3. just remove the B‘s view, only them
   * have different item-key。
   * @standardAction: experimental
   * @standardAction: keep
   * @supportVersion: 2.12
   **/

  @LynxProp(name = "item-holder-type", defaultInt = 0)
  public void setItemHolderType(Dynamic value) {
    mItemHolderType = ListEventManager.dynamicToInt(value, 0);
  }

  @LynxUIMethod
  public void removeStickyView() {
    if (mListStickyManager != null) {
      mListStickyManager.resetStickyView();
    }
  }

  /**
   * @name: enable-disappear
   * @description: enable send disAppear event
   * @category: temporary
   * @standardAction: offline
   * @supportVersion: 2.10
   **/
  @LynxProp(name = "enable-disappear", defaultBoolean = false)
  public void setEnableDisappear(boolean value) {
    mAppearEventCourier.setDisappear(value);
  }

  /**
   * @name: enable-size-cache
   * @description: In the case of water flow layout with PART_ON_LAYOUT thread strategy, when we
   * invoking obtainChild, we get child component immediately and measure the wrap view with
   * component's height and width which may be the wrong size, then onLayoutFinish() is invoked
   * and we get the correct size of component and remeasure the wrap view. These two measure process
   * may lead to flickering. This prop can be used in this situation, and we will use a size cache
   *to record all child's size. In that case, we can use a cached size in the first measure process
   * instead of a wrong size.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.11
   **/

  @LynxProp(name = "enable-size-cache", defaultBoolean = false)
  public void setEnableSizeCache(boolean value) {
    mEnableSizeCache = value;
  }

  /**
   * @name: enable-rtl
   * @description: If the value of "enable-rtl" is true, the layout direction will be the  rtl mode
   * @category: experimental
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = "enable-rtl", defaultBoolean = false)
  public void enableRtl(boolean value) {
    mEnableRTL = value;
  }

  @LynxProp(name = "enable-nested-scroll", defaultBoolean = true)
  public void enableNestedScroll(boolean value) {
    if (mView == null) {
      return;
    }
    mView.setNestedScrollingEnabled(value);
  }

  @LynxProp(name = "preload-buffer-count", defaultInt = 0)
  public void setPreloadBufferCount(Dynamic value) {
    mPreloadBufferCount = ListEventManager.dynamicToInt(value, 0);
  }

  /**
   * @name: max-fling-velocity-percent
   * @description: the max fling velocity of the recyclerView during flinging, the value should
   * between 0 and 1.
   * @category: experimental
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = "max-fling-velocity-percent", defaultDouble = 1.0)
  public void setMaxFlingVelocityPercent(float value) {
    mMaxFlingVelocityPercent = value;
  }

  @LynxUIMethod
  public void initCache() {
    if (mPreloadCache != null) {
      mPreloadCache.clear();
      mPreloadCache.initCache();
    }
  }

  boolean isPartOnLayoutThreadStrategy() {
    if (mContext == null || mContext.getLynxView() == null) {
      return false;
    }
    ThreadStrategyForRendering strategy = mContext.getLynxView().getThreadStrategyForRendering();
    return strategy == ThreadStrategyForRendering.PART_ON_LAYOUT;
  }

  /**
   * @name: android-enable-strict-scrollable
   * @description: the list's ancestor view can judge whether the list can scroll.when the
   * first/last component's height is 0 (if it is horizontal list, the width is 0 ), you should let
   * this value true. we we'll judge the border by
   * findFirstCompleteLyListItem/findLastCompleteLyListItem.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = "android-enable-strict-scrollable", defaultBoolean = false)
  public void setEnableStrictScrollable(boolean value) {
    mEnableStrictScrollable = value;
  }

  @LynxProp(name = "android-enable-focus-search", defaultBoolean = true)
  public void setEnableFocusSearch(boolean value) {
    mEnableFocusSearch = value;
  }

  @LynxProp(name = "android-enable-gap-item-decoration", defaultBoolean = false)
  public void setEnableGapItemDecoration(boolean value) {
    mEnableGapItemDecoration = value;
  }

  void updateGapItemDecorationIfNeed() {
    if (mView == null || mView.getLayoutManager() == null) {
      LLog.e(
          TAG, "Fail to update gap item decoration because mView == null or LayoutManager == null");
      return;
    }
    if (mEnableGapItemDecoration) {
      if (mGapItemDecoration == null) {
        mGapItemDecoration = new GapItemDecoration();
        mView.addItemDecoration(mGapItemDecoration);
      }
      mGapItemDecoration.setIsVertical(isVertical());
      mGapItemDecoration.setIsRTL(mEnableRTL && isRtl());
      mGapItemDecoration.setColumnCount(mColumnCount);
      mGapItemDecoration.setMainAxisGap(mMainAxisGap);
      mGapItemDecoration.setCrossAxisGap(mCrossAxisGap);
      mView.invalidateItemDecorations();
    } else {
      if (mGapItemDecoration != null) {
        mView.removeItemDecoration(mGapItemDecoration);
      }
    }
  }

  private void removeFrameCallback() {
    if (mFrameCallback != null) {
      Choreographer.getInstance().removeFrameCallback(mFrameCallback);
      mFrameCallback = null;
    }
  }

  public void setPreBufferListener() {
    if (mPreloadBufferCount > 0) {
      if (mPreloadListener == null) {
        mPreloadListener = new RecyclerView.OnScrollListener() {
          @Override
          public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
            super.onScrolled(recyclerView, dx, dy);
            if (getAdapter() != null && getAdapter().shouldInitCache) {
              if (mPreloadCache != null) {
                mPreloadCache.initCache();
              }
              getAdapter().shouldInitCache = false;
            }
          }
        };
        getView().addOnScrollListener(mPreloadListener);
      }
    } else {
      if (mPreloadCache != null) {
        mPreloadCache.clear();
      }
    }
  }

  /**
   * find first list item from the RecyclerView
   * @return
   */
  public int findFirstListItem() {
    int firstVisiblePosition = -1;
    RecyclerView.LayoutManager layoutManager = getView().getLayoutManager();
    if (layoutManager instanceof LinearLayoutManager) {
      firstVisiblePosition = ((LinearLayoutManager) layoutManager).findFirstVisibleItemPosition();
    } else if (layoutManager instanceof StaggeredGridLayoutManager) {
      if (mColumnCount > 0) {
        int[] spanIndexInfo = new int[mColumnCount];
        ((StaggeredGridLayoutManager) layoutManager).findFirstVisibleItemPositions(spanIndexInfo);
        firstVisiblePosition = spanIndexInfo[0];
        for (int i = 1; i < mColumnCount; i++) {
          firstVisiblePosition = Math.min(firstVisiblePosition, spanIndexInfo[i]);
        }
      }
    }
    return firstVisiblePosition;
  }

  /***
   * find last item from the RecyclerView
   * @return
   */
  public int findLastListItem() {
    int lastVisiblePosition = -1;
    RecyclerView.LayoutManager layoutManager = getView().getLayoutManager();
    if (layoutManager instanceof LinearLayoutManager) {
      lastVisiblePosition = ((LinearLayoutManager) layoutManager).findLastVisibleItemPosition();
    } else if (layoutManager instanceof StaggeredGridLayoutManager) {
      if (mColumnCount > 0) {
        int[] spanIndexInfo = new int[mColumnCount];
        ((StaggeredGridLayoutManager) layoutManager).findLastVisibleItemPositions(spanIndexInfo);
        for (int i = 0; i < mColumnCount; i++) {
          lastVisiblePosition = Math.max(lastVisiblePosition, spanIndexInfo[i]);
        }
      }
    }
    return lastVisiblePosition;
  }

  /***
   * find first completeLy list item from the RecyclerView.
   * @return
   */
  private int findFirstCompleteLyListItem() {
    if (getView() != null) {
      RecyclerView.LayoutManager layoutManager = getView().getLayoutManager();
      if (layoutManager instanceof LinearLayoutManager) {
        return ((LinearLayoutManager) layoutManager).findFirstCompletelyVisibleItemPosition();
      } else if (layoutManager instanceof StaggeredGridLayoutManager) {
        if (mColumnCount <= 0) {
          return -1;
        }
        int[] spanIndexInfo = new int[mColumnCount];
        ((StaggeredGridLayoutManager) layoutManager)
            .findFirstCompletelyVisibleItemPositions(spanIndexInfo);
        int result = spanIndexInfo[0];
        for (int i = 1; i < mColumnCount; i++) {
          result = Math.min(result, spanIndexInfo[i]);
        }
        return result;
      }
    }
    return -1;
  }

  /***
   * find last completeLy list item from the RecyclerView.
   * @return
   */

  private int findLastCompleteLyListItem() {
    if (getView() != null) {
      RecyclerView.LayoutManager layoutManager = getView().getLayoutManager();
      if (layoutManager instanceof LinearLayoutManager) {
        return ((LinearLayoutManager) layoutManager).findLastCompletelyVisibleItemPosition();
      } else if (layoutManager instanceof StaggeredGridLayoutManager) {
        if (mColumnCount <= 0) {
          return -1;
        }
        int[] spanIndexInfo = new int[mColumnCount];
        ((StaggeredGridLayoutManager) layoutManager)
            .findLastCompletelyVisibleItemPositions(spanIndexInfo);
        int result = -1;
        for (int i = 0; i < mColumnCount; i++) {
          result = Math.max(result, spanIndexInfo[i]);
        }
        return result;
      }
    }
    return -1;
  }
}
