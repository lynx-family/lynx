// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.view.MotionEvent;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import com.lynx.tasm.gesture.handler.GestureHandlerTrigger;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

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
    if (!isEnableNewGesture() || mGestureHandlerTrigger == null) {
      return;
    }
    mGestureHandlerTrigger.computeScroll(mCompeteChainCandidates);
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
}
