// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.widget.OverScroller;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import com.lynx.tasm.gesture.handler.GestureConstants;
import com.lynx.tasm.gesture.handler.GestureHandlerTrigger;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Manages the gesture arenas for handling touch events and dispatching them to the appropriate
 * members. Supports adding, removing, and updating gesture members, as well as resolving touch
 * events and determining the winner.
 */
public class GestureArenaManager {
  private static final String TAG = "GestureArenaManager";
  private Map<Integer, WeakReference<GestureArenaMember>> mArenaMemberMap;
  private LinkedList<GestureArenaMember> mCompeteChainCandidates;
  private LinkedList<GestureArenaMember> mBubbleCandidate;
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

  private static final int INPUT_DOWN = 0;
  private static final int INPUT_MOVE = 1;
  private static final int INPUT_UP = 2;
  private static final int INPUT_CANCEL = 3;
  private static final int INPUT_FLING_FRAME = 4;
  private static final int CORE_STATE_ACTIVE = 2;
  private static final int CORE_STATE_FAIL = 3;
  private static final int CORE_STATE_END = 4;
  private static final int CORE_STATE_CANCEL = 5;
  private static final int FLING_MIN = Integer.MIN_VALUE;
  private static final int FLING_MAX = Integer.MAX_VALUE;
  private static final int FLING_SPEED_THRESHOLD = 300;
  private static final long FLING_FRAME_DELAY_MS = 16;

  private boolean mUseUnifiedGestureHandler;
  private long mNativeGestureArena;
  private LynxContext mLynxContext;
  private OverScroller mUnifiedScroller;
  private VelocityTracker mUnifiedVelocityTracker;
  private long mUnifiedSequenceId;
  private int[] mUnifiedResponseChain = new int[0];
  private double mUnifiedLastPageX;
  private double mUnifiedLastPageY;
  private int mUnifiedLastFlingX;
  private int mUnifiedLastFlingY;
  private final Map<Long, Runnable> mUnifiedTimers = new HashMap<>();
  private final Set<Long> mUnifiedActiveHandlers = new HashSet<>();
  private final Runnable mUnifiedFlingRunnable = this::computeUnifiedFling;

  private GestureDetectorManager mGestureDetectorManager;

  private boolean mIsEnableNewGesture;

  private GestureArenaMember mWinner;

  private GestureHandlerTrigger mGestureHandlerTrigger;

  /**
   * Initializes the GestureArenaManager with the given LynxContext.
   *
   * @param enable if enable new gesture.
   * @param context The LynxContext used for initializing the manager.
   */
  public void init(boolean enable, LynxContext context) {
    init(enable, false, context);
  }

  public void init(boolean enable, boolean enableUnifiedGestureHandler, LynxContext context) {
    onDestroy();
    mIsEnableNewGesture = enable;
    if (!isEnableNewGesture()) {
      return;
    }
    mArenaMemberMap = new HashMap<>();
    mCompeteChainCandidates = new LinkedList<>();
    mBubbleCandidate = new LinkedList<>();
    mLynxContext = context;
    mUseUnifiedGestureHandler = false;
    if (enableUnifiedGestureHandler) {
      mNativeGestureArena = nativeCreateGestureArena();
      if (mNativeGestureArena != 0) {
        mUseUnifiedGestureHandler = true;
        mUnifiedScroller = new OverScroller(context);
        return;
      }
    }
    mGestureDetectorManager = new GestureDetectorManager(this);
    mGestureHandlerTrigger = new GestureHandlerTrigger(context, mGestureDetectorManager);
  }

  /**
   * Dispatches the touch event to the appropriate gesture arena member.
   *
   * @param event The MotionEvent to dispatch.
   * @param lynxTouchEvent The LynxTouchEvent associated with the event.
   */
  public void dispatchTouchEventToArena(MotionEvent event, LynxTouchEvent lynxTouchEvent) {
    if (!isEnableNewGesture()) {
      return;
    }
    if (mUseUnifiedGestureHandler) {
      dispatchTouchEventToUnifiedArena(event);
      return;
    }
    if (mGestureHandlerTrigger == null) {
      return;
    }
    mGestureHandlerTrigger.resolveTouchEvent(
        event, mCompeteChainCandidates, lynxTouchEvent, mBubbleCandidate);
  }

  public boolean isUsingUnifiedGestureHandler() {
    return isEnableNewGesture() && mUseUnifiedGestureHandler && mNativeGestureArena != 0;
  }

  private boolean isEnableNewGesture() {
    if (!LynxLiteConfigs.enableNewGesture()) {
      return false;
    }
    return mIsEnableNewGesture;
  }

  /**
   * Dispatches the bubble touch event to the appropriate gesture arena member.
   *
   * @param type The type of the touch event.
   * @param touchEvent The LynxTouchEvent to dispatch.
   */
  public void dispatchBubbleTouchEvent(String type, LynxTouchEvent touchEvent) {
    if (!isEnableNewGesture() || mUseUnifiedGestureHandler || mGestureHandlerTrigger == null) {
      return;
    }
    mGestureHandlerTrigger.dispatchBubbleTouchEvent(type, touchEvent, mBubbleCandidate, mWinner);
  }

  /**
   * Sets the active UI member of the arena when a down event occurs.
   *
   * @param target The EventTarget associated with the active UI member.
   */
  public void setActiveUIToArenaAtDownEvent(EventTarget target) {
    if (!isEnableNewGesture()) {
      return;
    }
    clearCurrentGesture();
    if (mArenaMemberMap == null || mArenaMemberMap.isEmpty()
        || (!mUseUnifiedGestureHandler && mGestureHandlerTrigger == null)) {
      return;
    }

    EventTarget temp = target;
    while (temp != null) {
      for (WeakReference<GestureArenaMember> weakRef : mArenaMemberMap.values()) {
        if (weakRef == null) {
          continue;
        }
        GestureArenaMember member = weakRef.get();
        if (member == null) {
          continue;
        }
        if (member.getGestureArenaMemberId() > 0
            && member.getGestureArenaMemberId() == temp.getGestureArenaMemberId()) {
          mBubbleCandidate.add(member);
        }
      }
      temp = temp.parent();
    }
    if (mGestureDetectorManager != null) {
      mCompeteChainCandidates =
          mGestureDetectorManager.convertResponseChainToCompeteChain(mBubbleCandidate);
    }

    if (mCompeteChainCandidates != null && !mCompeteChainCandidates.isEmpty()) {
      mWinner = mCompeteChainCandidates.getFirst();
    }
    if (mUseUnifiedGestureHandler) {
      mUnifiedResponseChain = new int[mBubbleCandidate.size()];
      for (int i = 0; i < mBubbleCandidate.size(); i++) {
        mUnifiedResponseChain[i] = mBubbleCandidate.get(i).getGestureArenaMemberId();
      }
    } else {
      mGestureHandlerTrigger.initCurrentWinnerWhenDown(mWinner);
    }
  }

  /**
   * Computes the scroll for the active gesture members.
   */
  public void computeScroll() {
    if (!isEnableNewGesture() || mUseUnifiedGestureHandler || mGestureHandlerTrigger == null) {
      return;
    }

    mMainThreadHandler.post(() -> {
      if (mGestureHandlerTrigger != null) {
        mGestureHandlerTrigger.computeScroll(mCompeteChainCandidates);
      }
    });
  }

  /**
   * Clears the current gesture state.
   */
  private void clearCurrentGesture() {
    mWinner = null;
    mUnifiedResponseChain = new int[0];
    if (mBubbleCandidate != null) {
      mBubbleCandidate.clear();
    }
    if (mCompeteChainCandidates != null) {
      mCompeteChainCandidates.clear();
    }
  }

  /**
   * Adds a gesture member to the arena.
   *
   * @param member The GestureArenaMember to add.
   * @return The assigned member ID.
   */
  @MainThread
  public int addMember(@Nullable GestureArenaMember member) {
    if (!isEnableNewGesture() || member == null || mArenaMemberMap == null) {
      return 0;
    }
    if (mUseUnifiedGestureHandler && !hasSupportedGesture(member.getGestureDetectorMap())) {
      return 0;
    }
    mArenaMemberMap.put(member.getSign(), new WeakReference<>(member));
    registerGestureDetectors(member.getSign(), member.getGestureDetectorMap());
    return member.getSign();
  }

  /**
   * Checks if a gesture member with the given member ID exists in the arena.
   *
   * @param memberId The member ID to check.
   * @return True if the member exists, false otherwise.
   */
  public boolean isMemberExist(int memberId) {
    if (!isEnableNewGesture() || mArenaMemberMap == null) {
      return false;
    }
    return mArenaMemberMap.containsKey(memberId);
  }

  /**
   * Sets the state of the gesture detector associated with the given member ID.
   *
   * @param memberId The member ID of the gesture detector.
   * @param gestureId The ID of the gesture.
   * @param state The state of the gesture.
   */
  public void setGestureDetectorState(int memberId, int gestureId, int state) {
    if (!isEnableNewGesture() || mArenaMemberMap == null) {
      return;
    }
    if (mUseUnifiedGestureHandler) {
      if (mNativeGestureArena != 0 && mArenaMemberMap.containsKey(memberId)) {
        nativeSetGestureState(mNativeGestureArena, memberId, gestureId, state);
      }
      return;
    }
    if (mGestureHandlerTrigger == null) {
      return;
    }
    WeakReference<GestureArenaMember> weakRef = mArenaMemberMap.get(memberId);
    mGestureHandlerTrigger.handleGestureDetectorState(
        weakRef == null ? null : weakRef.get(), gestureId, state);
  }

  public boolean hasActivePlatformGesture() {
    if (mUseUnifiedGestureHandler) {
      return isUsingUnifiedGestureHandler() && !mUnifiedActiveHandlers.isEmpty();
    }
    return isEnableNewGesture() && mGestureHandlerTrigger != null
        && mGestureHandlerTrigger.hasActivePlatformGesture();
  }

  /**
   * Removes a gesture member from the arena.
   *
   * @param member The GestureArenaMember to remove.
   */
  @MainThread
  public void removeMember(@Nullable GestureArenaMember member) {
    if (!isEnableNewGesture() || member == null || mArenaMemberMap == null) {
      return;
    }
    mArenaMemberMap.remove(member.getGestureArenaMemberId());
    if (mUseUnifiedGestureHandler) {
      if (mNativeGestureArena != 0) {
        nativeRemoveMember(mNativeGestureArena, member.getGestureArenaMemberId());
      }
      removeActiveHandlersForMember(member.getGestureArenaMemberId());
    } else {
      unRegisterGestureDetectors(member.getGestureArenaMemberId(), member.getGestureDetectorMap());
    }
  }

  /**
   * Retrieves the gesture arena member with the given ID.
   *
   * @param id The ID of the member.
   * @return The GestureArenaMember with the given ID.
   */
  @Nullable
  public GestureArenaMember getMemberById(int id) {
    if (mArenaMemberMap == null) {
      return null;
    }
    WeakReference<GestureArenaMember> member = mArenaMemberMap.get(id);
    if (member != null) {
      return member.get();
    } else {
      return null;
    }
  }

  /**
   * Cleans up the GestureArenaManager and releases any resources.
   */
  public void onDestroy() {
    stopUnifiedFling();
    recycleUnifiedVelocityTracker();
    for (Runnable timer : mUnifiedTimers.values()) {
      mMainThreadHandler.removeCallbacks(timer);
    }
    mUnifiedTimers.clear();
    mUnifiedActiveHandlers.clear();
    if (mNativeGestureArena != 0) {
      long nativeGestureArena = mNativeGestureArena;
      mNativeGestureArena = 0;
      nativeDestroyGestureArena(nativeGestureArena);
    }
    if (mArenaMemberMap != null) {
      mArenaMemberMap.clear();
    }
    if (mCompeteChainCandidates != null) {
      mCompeteChainCandidates.clear();
    }
    if (mBubbleCandidate != null) {
      mBubbleCandidate.clear();
    }
    if (mGestureDetectorManager != null) {
      mGestureDetectorManager.onDestroy();
    }
    if (mGestureHandlerTrigger != null) {
      mGestureHandlerTrigger.onDestroy();
    }
    mGestureDetectorManager = null;
    mGestureHandlerTrigger = null;
    mUnifiedScroller = null;
    mLynxContext = null;
    mUseUnifiedGestureHandler = false;
    mUnifiedResponseChain = new int[0];
  }

  /**
   * Registers gesture detectors for the given member ID.
   *
   * @param memberId The member ID to register the detectors for.
   * @param gestureDetectors The map of gesture detectors to register.
   */
  @MainThread
  public void registerGestureDetectors(
      int memberId, Map<Integer, GestureDetector> gestureDetectors) {
    if (!isEnableNewGesture()) {
      return;
    }

    if (mUseUnifiedGestureHandler) {
      replaceUnifiedGestureDetectors(memberId, gestureDetectors);
      return;
    }

    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return;
    }

    if (mGestureDetectorManager != null) {
      for (Map.Entry<Integer, GestureDetector> entry : gestureDetectors.entrySet()) {
        GestureDetector detector = entry.getValue();
        mGestureDetectorManager.registerGestureDetector(memberId, detector);
      }
    }
  }

  /**
   * Unregisters gesture detectors for the given member ID.
   *
   * @param memberId The member ID to unregister the detectors for.
   * @param gestureDetectors The map of gesture detectors to unregister.
   */
  @MainThread
  public void unRegisterGestureDetectors(
      int memberId, Map<Integer, GestureDetector> gestureDetectors) {
    if (!isEnableNewGesture()) {
      return;
    }

    if (mUseUnifiedGestureHandler) {
      if (mNativeGestureArena != 0) {
        nativeRemoveMember(mNativeGestureArena, memberId);
      }
      return;
    }

    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return;
    }
    if (mGestureDetectorManager != null) {
      for (Map.Entry<Integer, GestureDetector> entry : gestureDetectors.entrySet()) {
        GestureDetector detector = entry.getValue();
        mGestureDetectorManager.unregisterGestureDetector(memberId, detector);
      }
    }
  }

  @MainThread
  public void replaceGestureDetectors(
      int memberId, @Nullable Map<Integer, GestureDetector> gestureDetectors) {
    if (!isUsingUnifiedGestureHandler() || mArenaMemberMap == null
        || !mArenaMemberMap.containsKey(memberId)) {
      return;
    }
    replaceUnifiedGestureDetectors(memberId, gestureDetectors);
  }

  private void dispatchTouchEventToUnifiedArena(MotionEvent event) {
    if (mNativeGestureArena == 0) {
      return;
    }
    int inputType;
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN:
        inputType = INPUT_DOWN;
        mUnifiedSequenceId++;
        resetUnifiedVelocityTracker();
        break;
      case MotionEvent.ACTION_MOVE:
        inputType = INPUT_MOVE;
        break;
      case MotionEvent.ACTION_UP:
        inputType = INPUT_UP;
        break;
      case MotionEvent.ACTION_CANCEL:
        inputType = INPUT_CANCEL;
        break;
      default:
        return;
    }

    if (mUnifiedVelocityTracker != null && inputType != INPUT_CANCEL) {
      mUnifiedVelocityTracker.addMovement(event);
    }

    double density = getDensity();
    double velocityX = 0;
    double velocityY = 0;
    if (inputType == INPUT_UP && mUnifiedVelocityTracker != null) {
      mUnifiedVelocityTracker.computeCurrentVelocity(1000);
      velocityX = mUnifiedVelocityTracker.getXVelocity(event.getPointerId(0)) / density;
      velocityY = mUnifiedVelocityTracker.getYVelocity(event.getPointerId(0)) / density;
    }

    double pageX = event.getX() / density;
    double pageY = event.getY() / density;
    double clientX = event.getRawX() / density;
    double clientY = event.getRawY() / density;
    double deltaX = inputType == INPUT_MOVE ? mUnifiedLastPageX - pageX : 0;
    double deltaY = inputType == INPUT_MOVE ? mUnifiedLastPageY - pageY : 0;
    if (inputType == INPUT_DOWN || inputType == INPUT_MOVE) {
      mUnifiedLastPageX = pageX;
      mUnifiedLastPageY = pageY;
    }
    nativeHandleInput(mNativeGestureArena, inputType, mUnifiedSequenceId, event.getPointerId(0),
        event.getEventTime(), getEpochTime(event), event.getRawX() / density,
        event.getRawY() / density, pageX, pageY, clientX, clientY, deltaX, deltaY, velocityX,
        velocityY, false, inputType == INPUT_DOWN ? mUnifiedResponseChain : null);

    if (inputType == INPUT_UP || inputType == INPUT_CANCEL) {
      recycleUnifiedVelocityTracker();
    }
  }

  private void resetUnifiedVelocityTracker() {
    if (mUnifiedVelocityTracker == null) {
      mUnifiedVelocityTracker = VelocityTracker.obtain();
    } else {
      mUnifiedVelocityTracker.clear();
    }
  }

  private void recycleUnifiedVelocityTracker() {
    if (mUnifiedVelocityTracker == null) {
      return;
    }
    mUnifiedVelocityTracker.recycle();
    mUnifiedVelocityTracker = null;
  }

  private long getEpochTime(MotionEvent event) {
    return System.currentTimeMillis() - (SystemClock.uptimeMillis() - event.getEventTime());
  }

  private double getDensity() {
    if (mLynxContext == null || mLynxContext.getResources() == null
        || mLynxContext.getResources().getDisplayMetrics() == null) {
      return 1;
    }
    return mLynxContext.getResources().getDisplayMetrics().density;
  }

  private void replaceUnifiedGestureDetectors(
      int memberId, @Nullable Map<Integer, GestureDetector> gestureDetectors) {
    if (mNativeGestureArena == 0) {
      return;
    }
    List<GestureDetector> detectors = new ArrayList<>();
    if (gestureDetectors != null) {
      for (GestureDetector detector : gestureDetectors.values()) {
        if (detector != null && isSupportedGestureType(detector.getGestureType())) {
          detectors.add(detector);
        }
      }
    }
    detectors.sort((left, right) -> Integer.compare(left.getGestureID(), right.getGestureID()));

    List<Integer> encoded = new ArrayList<>();
    double[] configs = new double[detectors.size() * 5];
    for (int i = 0; i < detectors.size(); i++) {
      GestureDetector detector = detectors.get(i);
      List<Integer> simultaneous = getRelation(detector, GestureDetector.SIMULTANEOUS);
      List<Integer> waitFor = getRelation(detector, GestureDetector.WAIT_FOR);
      List<Integer> continueWith = getRelation(detector, GestureDetector.CONTINUE_WITH);
      encoded.add(detector.getGestureID());
      encoded.add(detector.getGestureType());
      encoded.add(getCallbackMask(detector));
      encoded.add(simultaneous.size());
      encoded.add(waitFor.size());
      encoded.add(continueWith.size());
      encoded.addAll(simultaneous);
      encoded.addAll(waitFor);
      encoded.addAll(continueWith);

      ReadableMap config = detector.getConfigMap();
      int offset = i * 5;
      configs[offset] = getConfig(config, GestureConstants.MIN_DISTANCE, 0);
      configs[offset + 1] = getConfig(config, GestureConstants.MAX_DISTANCE, 10);
      configs[offset + 2] = getConfig(config, GestureConstants.MIN_DURATION, 500);
      configs[offset + 3] = getConfig(config, GestureConstants.MAX_DURATION, 500);
      configs[offset + 4] = getConfig(config, GestureConstants.TAP_SLOP, 3);
    }

    int[] gestureData = new int[encoded.size()];
    for (int i = 0; i < encoded.size(); i++) {
      gestureData[i] = encoded.get(i);
    }
    nativeReplaceMemberGestures(
        mNativeGestureArena, memberId, detectors.size(), gestureData, configs);
  }

  private boolean hasSupportedGesture(@Nullable Map<Integer, GestureDetector> gestureDetectors) {
    if (gestureDetectors == null) {
      return false;
    }
    for (GestureDetector detector : gestureDetectors.values()) {
      if (detector != null && isSupportedGestureType(detector.getGestureType())) {
        return true;
      }
    }
    return false;
  }

  private boolean isSupportedGestureType(int type) {
    return type == GestureDetector.GESTURE_TYPE_PAN || type == GestureDetector.GESTURE_TYPE_FLING
        || type == GestureDetector.GESTURE_TYPE_DEFAULT || type == GestureDetector.GESTURE_TYPE_TAP
        || type == GestureDetector.GESTURE_TYPE_LONG_PRESS
        || type == GestureDetector.GESTURE_TYPE_NATIVE;
  }

  private List<Integer> getRelation(GestureDetector detector, String relationName) {
    List<Integer> relation = detector.getRelationMap().get(relationName);
    return relation == null ? new ArrayList<>() : relation;
  }

  private int getCallbackMask(GestureDetector detector) {
    int mask = 0;
    for (String callback : detector.getGestureCallbackNames()) {
      switch (callback) {
        case GestureConstants.ON_TOUCHES_DOWN:
          mask |= 1;
          break;
        case GestureConstants.ON_TOUCHES_MOVE:
          mask |= 1 << 1;
          break;
        case GestureConstants.ON_TOUCHES_UP:
          mask |= 1 << 2;
          break;
        case GestureConstants.ON_TOUCHES_CANCEL:
          mask |= 1 << 3;
          break;
        case GestureConstants.ON_BEGIN:
          mask |= 1 << 4;
          break;
        case GestureConstants.ON_START:
          mask |= 1 << 5;
          break;
        case GestureConstants.ON_UPDATE:
          mask |= 1 << 6;
          break;
        case GestureConstants.ON_END:
          mask |= 1 << 7;
          break;
        default:
          break;
      }
    }
    return mask;
  }

  private double getConfig(@Nullable ReadableMap config, String name, double defaultValue) {
    return config == null ? defaultValue : config.getDouble(name, defaultValue);
  }

  private long handlerKey(int memberId, int gestureId) {
    return ((long) memberId << 32) ^ (gestureId & 0xffffffffL);
  }

  private void removeActiveHandlersForMember(int memberId) {
    long memberPrefix = (long) memberId << 32;
    mUnifiedActiveHandlers.removeIf(key -> (key & 0xffffffff00000000L) == memberPrefix);
  }

  @CalledByNative
  private boolean isMemberValid(int memberId) {
    GestureArenaMember member = getMemberById(memberId);
    if (member == null && mArenaMemberMap != null) {
      mArenaMemberMap.remove(memberId);
    }
    return member != null;
  }

  @CalledByNative
  private double[] convertPageToMember(int memberId, double pageX, double pageY) {
    GestureArenaMember member = getMemberById(memberId);
    if (!(member instanceof LynxBaseUI) || mLynxContext == null
        || mLynxContext.getLynxUIOwner() == null
        || mLynxContext.getLynxUIOwner().getRootUI() == null) {
      return new double[] {pageX, pageY};
    }
    LynxBaseUI ui = (LynxBaseUI) member;
    double density = getDensity();
    PointF pagePoint = new PointF((float) (pageX * density), (float) (pageY * density));
    PointF localPoint;
    if (mLynxContext.getEnableTransformedTouchPosition()) {
      localPoint = LynxUIHelper.convertPointFromUIToAnotherUI(
          mLynxContext.getLynxUIOwner().getRootUI(), ui, pagePoint);
    } else {
      RectF rect = LynxUIHelper.convertRectFromUIToRootUI(
          ui, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      localPoint = new PointF(pagePoint.x - rect.left, pagePoint.y - rect.top);
    }
    return new double[] {localPoint.x / density, localPoint.y / density};
  }

  @CalledByNative
  private double[] getScrollState(int memberId) {
    GestureArenaMember member = getMemberById(memberId);
    if (member == null) {
      return new double[4];
    }
    double density = getDensity();
    return new double[] {member.getMemberScrollX() / density, member.getMemberScrollY() / density,
        member.isAtBorder(true) ? 1 : 0, member.isAtBorder(false) ? 1 : 0};
  }

  @CalledByNative
  private int getScrollDirection(int memberId) {
    GestureArenaMember member = getMemberById(memberId);
    return member == null ? GestureConstants.DIRECTION_UNDETERMINED
                          : member.getScrollContainerDirection();
  }

  @CalledByNative
  private boolean canConsumeGesture(int memberId, int direction, double deltaX, double deltaY) {
    GestureArenaMember member = getMemberById(memberId);
    double density = getDensity();
    return member != null
        && member.canConsumeGesture((float) (deltaX * density), (float) (deltaY * density));
  }

  @CalledByNative
  private boolean shouldConsumeGesture(int memberId) {
    GestureArenaMember member = getMemberById(memberId);
    return member != null && member.shouldConsumeGesture();
  }

  @CalledByNative
  private double[] scrollBy(int memberId, double deltaX, double deltaY) {
    GestureArenaMember member = getMemberById(memberId);
    if (member == null) {
      return new double[] {0, 0, deltaX, deltaY};
    }
    double density = getDensity();
    int beforeX = member.getMemberScrollX();
    int beforeY = member.getMemberScrollY();
    member.onGestureScrollBy((float) (deltaX * density), (float) (deltaY * density));
    double consumedX = (member.getMemberScrollX() - beforeX) / density;
    double consumedY = (member.getMemberScrollY() - beforeY) / density;
    return new double[] {consumedX, consumedY, deltaX - consumedX, deltaY - consumedY};
  }

  @CalledByNative
  private void onGestureRecognized(int memberId) {
    GestureArenaMember member = getMemberById(memberId);
    if (mLynxContext != null && member instanceof LynxBaseUI) {
      mLynxContext.onGestureRecognized((LynxBaseUI) member);
    }
  }

  @CalledByNative
  private void onGestureStateChanged(int memberId, int gestureId, int state) {
    long key = handlerKey(memberId, gestureId);
    if (state == CORE_STATE_ACTIVE) {
      mUnifiedActiveHandlers.add(key);
    } else if (state == CORE_STATE_FAIL || state == CORE_STATE_END || state == CORE_STATE_CANCEL) {
      mUnifiedActiveHandlers.remove(key);
    }
    GestureArenaMember member = getMemberById(memberId);
    if (member != null) {
      member.onPlatformGestureStatusChanged(toPlatformState(state));
    }
  }

  private int toPlatformState(int state) {
    switch (state) {
      case 0:
        return GestureConstants.LYNX_STATE_INIT;
      case 1:
        return GestureConstants.LYNX_STATE_BEGIN;
      case CORE_STATE_ACTIVE:
        return GestureConstants.LYNX_STATE_ACTIVE;
      case CORE_STATE_FAIL:
        return GestureConstants.LYNX_STATE_FAIL;
      case CORE_STATE_END:
        return GestureConstants.LYNX_STATE_END;
      case CORE_STATE_CANCEL:
        return GestureConstants.LYNX_STATE_CANCELLED;
      default:
        return GestureConstants.LYNX_STATE_UNDETERMINED;
    }
  }

  @CalledByNative
  private void dispatchGestureEvent(int memberId, int gestureId, int callbackType, int sourceType,
      long timestamp, double localX, double localY, double pageX, double pageY, double clientX,
      double clientY, double deltaX, double deltaY, double scrollX, double scrollY,
      boolean isAtStart, boolean isAtEnd) {
    if (mLynxContext == null || mLynxContext.getEventEmitter() == null) {
      return;
    }
    String callbackName = getCallbackName(callbackType);
    if (callbackName == null) {
      return;
    }
    HashMap<String, Object> params = new HashMap<>();
    params.put("timestamp", timestamp);
    String sourceName = getSourceName(sourceType);
    if (sourceName != null) {
      params.put("type", sourceName);
    }
    params.put("x", localX);
    params.put("y", localY);
    params.put("pageX", pageX);
    params.put("pageY", pageY);
    params.put("clientX", clientX);
    params.put("clientY", clientY);
    params.put("deltaX", deltaX);
    params.put("deltaY", deltaY);
    params.put("scrollX", scrollX);
    params.put("scrollY", scrollY);
    params.put("isAtStart", isAtStart);
    params.put("isAtEnd", isAtEnd);
    LynxCustomEvent event = new LynxCustomEvent(memberId, callbackName, params);
    mLynxContext.getEventEmitter().sendGestureEvent(gestureId, event);
  }

  @Nullable
  private String getCallbackName(int callbackType) {
    switch (callbackType) {
      case 0:
        return GestureConstants.ON_TOUCHES_DOWN;
      case 1:
        return GestureConstants.ON_TOUCHES_MOVE;
      case 2:
        return GestureConstants.ON_TOUCHES_UP;
      case 3:
        return GestureConstants.ON_TOUCHES_CANCEL;
      case 4:
        return GestureConstants.ON_BEGIN;
      case 5:
        return GestureConstants.ON_START;
      case 6:
        return GestureConstants.ON_UPDATE;
      case 7:
        return GestureConstants.ON_END;
      default:
        return null;
    }
  }

  @Nullable
  private String getSourceName(int sourceType) {
    switch (sourceType) {
      case INPUT_DOWN:
        return LynxTouchEvent.EVENT_TOUCH_START;
      case INPUT_MOVE:
        return LynxTouchEvent.EVENT_TOUCH_MOVE;
      case INPUT_UP:
        return LynxTouchEvent.EVENT_TOUCH_END;
      case INPUT_CANCEL:
        return LynxTouchEvent.EVENT_TOUCH_CANCEL;
      case INPUT_FLING_FRAME:
        return "unknown";
      default:
        return null;
    }
  }

  @CalledByNative
  private void scheduleTimer(long token, double delayMs) {
    cancelTimer(token);
    Runnable timer = () -> {
      mUnifiedTimers.remove(token);
      if (mNativeGestureArena != 0) {
        nativeHandleTimer(
            mNativeGestureArena, token, SystemClock.uptimeMillis(), System.currentTimeMillis());
      }
    };
    mUnifiedTimers.put(token, timer);
    mMainThreadHandler.postDelayed(timer, Math.max(0, (long) Math.ceil(delayMs)));
  }

  @CalledByNative
  private void cancelTimer(long token) {
    Runnable timer = mUnifiedTimers.remove(token);
    if (timer != null) {
      mMainThreadHandler.removeCallbacks(timer);
    }
  }

  @CalledByNative
  private boolean startFling(double velocityX, double velocityY) {
    if (mUnifiedScroller == null
        || (Math.abs(velocityX) <= FLING_SPEED_THRESHOLD
            && Math.abs(velocityY) <= FLING_SPEED_THRESHOLD)) {
      return false;
    }
    stopUnifiedFling();
    mUnifiedLastFlingX = 0;
    mUnifiedLastFlingY = 0;
    double density = getDensity();
    mUnifiedScroller.fling(0, 0, (int) (-velocityX * density), (int) (-velocityY * density),
        FLING_MIN, FLING_MAX, FLING_MIN, FLING_MAX);
    mMainThreadHandler.post(mUnifiedFlingRunnable);
    return true;
  }

  private void computeUnifiedFling() {
    if (mNativeGestureArena == 0 || mUnifiedScroller == null) {
      return;
    }
    boolean hasFrame = mUnifiedScroller.computeScrollOffset();
    int currentX = hasFrame ? mUnifiedScroller.getCurrX() : mUnifiedLastFlingX;
    int currentY = hasFrame ? mUnifiedScroller.getCurrY() : mUnifiedLastFlingY;
    double density = getDensity();
    boolean finished = !hasFrame || mUnifiedScroller.isFinished();
    nativeHandleInput(mNativeGestureArena, INPUT_FLING_FRAME, mUnifiedSequenceId, 0,
        SystemClock.uptimeMillis(), System.currentTimeMillis(), 0, 0, 0, 0, 0, 0,
        (currentX - mUnifiedLastFlingX) / density, (currentY - mUnifiedLastFlingY) / density, 0, 0,
        finished, null);
    mUnifiedLastFlingX = currentX;
    mUnifiedLastFlingY = currentY;
    if (!finished) {
      mMainThreadHandler.postDelayed(mUnifiedFlingRunnable, FLING_FRAME_DELAY_MS);
    }
  }

  @CalledByNative
  private void stopFling() {
    stopUnifiedFling();
  }

  private void stopUnifiedFling() {
    mMainThreadHandler.removeCallbacks(mUnifiedFlingRunnable);
    if (mUnifiedScroller != null && !mUnifiedScroller.isFinished()) {
      mUnifiedScroller.abortAnimation();
    }
  }

  private native long nativeCreateGestureArena();
  private native void nativeDestroyGestureArena(long nativePtr);
  private native void nativeReplaceMemberGestures(
      long nativePtr, int memberId, int gestureCount, int[] gestureData, double[] configs);
  private native void nativeRemoveMember(long nativePtr, int memberId);
  private native void nativeHandleInput(long nativePtr, int type, long sequenceId, int pointerId,
      double monotonicTime, long epochTime, double screenX, double screenY, double pageX,
      double pageY, double clientX, double clientY, double deltaX, double deltaY, double velocityX,
      double velocityY, boolean flingFinished, int[] responseChain);
  private native void nativeHandleTimer(
      long nativePtr, long token, double monotonicTime, long epochTime);
  private native void nativeSetGestureState(long nativePtr, int memberId, int gestureId, int state);
}
