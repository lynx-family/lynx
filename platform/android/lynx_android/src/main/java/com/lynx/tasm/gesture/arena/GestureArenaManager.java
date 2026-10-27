// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.widget.OverScroller;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
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

  private boolean mUseUnifiedGestureHandler;
  private long mNativeGestureArena;
  private LynxContext mLynxContext;
  private OverScroller mUnifiedScroller;
  private Choreographer mUnifiedFlingChoreographer;
  private VelocityTracker mUnifiedVelocityTracker;
  private long mUnifiedSequenceId;
  private boolean mUnifiedDispatchTouchMove;
  private int[] mUnifiedResponseChain = new int[0];
  private double mUnifiedLastPageX;
  private double mUnifiedLastPageY;
  private int mUnifiedLastFlingX;
  private int mUnifiedLastFlingY;
  private final Map<Long, Runnable> mUnifiedTimers = new HashMap<>();
  private final Set<Long> mUnifiedActiveHandlers = new HashSet<>();

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
    mIsEnableNewGesture = enable;
    if (!isEnableNewGesture()) {
      return;
    }
    mGestureDetectorManager = new GestureDetectorManager(this);
    mArenaMemberMap = new HashMap<>();
    mCompeteChainCandidates = new LinkedList<>();
    mBubbleCandidate = new LinkedList<>();
    mGestureHandlerTrigger = new GestureHandlerTrigger(context, mGestureDetectorManager);
  }

  /**
   * Dispatches the touch event to the appropriate gesture arena member.
   *
   * @param event The MotionEvent to dispatch.
   * @param lynxTouchEvent The LynxTouchEvent associated with the event.
   */
  public void dispatchTouchEventToArena(MotionEvent event, LynxTouchEvent lynxTouchEvent) {
    if (!isEnableNewGesture() || mGestureHandlerTrigger == null) {
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
    if (!isEnableNewGesture() || mGestureHandlerTrigger == null) {
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
    if (mArenaMemberMap == null || mArenaMemberMap.isEmpty() || mGestureHandlerTrigger == null) {
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
    mGestureHandlerTrigger.initCurrentWinnerWhenDown(mWinner);
  }

  /**
   * Computes the scroll for the active gesture members.
   */
  public void computeScroll() {
    if (!isEnableNewGesture() || mGestureHandlerTrigger == null || mMainThreadHandler == null) {
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
    if (!isEnableNewGesture() || mGestureHandlerTrigger == null || mArenaMemberMap == null) {
      return;
    }
    WeakReference<GestureArenaMember> weakRef = mArenaMemberMap.get(memberId);
    mGestureHandlerTrigger.handleGestureDetectorState(weakRef.get(), gestureId, state);
  }

  public boolean hasActivePlatformGesture() {
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
    unRegisterGestureDetectors(member.getGestureArenaMemberId(), member.getGestureDetectorMap());
  }

  /**
   * Retrieves the gesture arena member with the given ID.
   *
   * @param id The ID of the member.
   * @return The GestureArenaMember with the given ID.
   */
  @Nullable
  public GestureArenaMember getMemberById(int id) {
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
    EncodedDetectors encoded = encodeGestureDetectors(gestureDetectors);
    nativeReplaceMemberGestures(
        mNativeGestureArena, memberId, encoded.count, encoded.data, encoded.configs);
  }

  // Keep the JNI record format testable before the native arena is enabled.
  static final class EncodedDetectors {
    final int count;
    final int[] data;
    final double[] configs;

    EncodedDetectors(int count, int[] data, double[] configs) {
      this.count = count;
      this.data = data;
      this.configs = configs;
    }
  }

  static EncodedDetectors encodeGestureDetectors(
      @Nullable Map<Integer, GestureDetector> gestureDetectors) {
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
    return new EncodedDetectors(detectors.size(), gestureData, configs);
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

  private static boolean isSupportedGestureType(int type) {
    return type == GestureDetector.GESTURE_TYPE_PAN || type == GestureDetector.GESTURE_TYPE_FLING
        || type == GestureDetector.GESTURE_TYPE_DEFAULT || type == GestureDetector.GESTURE_TYPE_TAP
        || type == GestureDetector.GESTURE_TYPE_LONG_PRESS
        || type == GestureDetector.GESTURE_TYPE_NATIVE;
  }

  private static List<Integer> getRelation(GestureDetector detector, String relationName) {
    List<Integer> relation = detector.getRelationMap().get(relationName);
    return relation == null ? new ArrayList<>() : relation;
  }

  private static int getCallbackMask(GestureDetector detector) {
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

  private static double getConfig(@Nullable ReadableMap config, String name, double defaultValue) {
    return config == null ? defaultValue : config.getDouble(name, defaultValue);
  }

  private long handlerKey(int memberId, int gestureId) {
    return ((long) memberId << 32) ^ (gestureId & 0xffffffffL);
  }

  private void removeActiveHandlersForMember(int memberId) {
    long memberPrefix = (long) memberId << 32;
    mUnifiedActiveHandlers.removeIf(key -> (key & 0xffffffff00000000L) == memberPrefix);
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
