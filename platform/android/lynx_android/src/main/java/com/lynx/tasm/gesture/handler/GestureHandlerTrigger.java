// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import android.util.Pair;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.widget.OverScroller;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import java.util.*;

/**
 * This class represents a Gesture Handler Trigger that manages touch gestures and dispatches events
 * to appropriate gesture handlers. It facilitates the recognition and handling of touch events
 * and manages the state of the active gestures. The class coordinates interactions between
 * various gesture detectors and their associated handlers.
 *
 * The GestureHandlerTrigger is responsible for identifying the current winner of the touch event,
 * updating simultaneous winners, computing scrolls, and dispatching events to the respective
 * gesture handlers based on the type of gesture and event.
 *
 * The class maintains a list of GestureArenaMembers to compete with and handles the bubbling of
 * touch events to the corresponding gesture handlers.
 *
 * This class is typically used in conjunction with GestureDetectorManager to coordinate touch
 * interactions and support complex gesture handling in various applications.
 */
public class GestureHandlerTrigger {
  private static final String TAG = "GestureHandlerTrigger";

  private final OverScroller mScroller;

  private LynxContext mContext;
  private int mLastFlingScrollX;
  private int mLastFlingScrollY;
  private int mLastFlingTargetId;
  private VelocityTracker mVelocityTracker;

  private HashSet<GestureArenaMember> mSimultaneousWinners;
  // record the ids of the current winner's simultaneous gesture
  private HashSet<Integer> mSimultaneousGestureIds;

  // record duplicated member when use continuesWith, such as A -> B -> A -> C
  private GestureArenaMember mDuplicatedMember;

  @Nullable private GestureArenaMember mWinner;

  // last winner win gesture, used to inform last winner onEnd callback if next winner is active
  @Nullable private GestureArenaMember mLastWinner;

  // last fling winner
  private GestureArenaMember mLastFlingWinner;

  private final GestureDetectorManager mGestureDetectorManager;

  /**
   * Constructs a GestureHandlerTrigger instance.
   *
   * @param context The LynxContext instance.
   * @param manager The GestureDetectorManager instance.
   */
  public GestureHandlerTrigger(LynxContext context, GestureDetectorManager manager) {
    mScroller = new OverScroller(context);
    mContext = context;
    mGestureDetectorManager = manager;
  }

  /**
   * Initializes the current winner when a touchdown event occurs.
   *
   * @param member The GestureArenaMember representing the current winner.
   */
  public void initCurrentWinnerWhenDown(GestureArenaMember member) {
    mWinner = member;
    updateLastWinner(mWinner);
    updateSimultaneousWinner(mWinner);
    resetGestureHandlerAndSimultaneous(mWinner);
  }

  /**
   * Resolves the touch event and dispatches appropriate events to gesture handlers.
   *
   * @param event                      The MotionEvent to be resolved.
   * @param competeChainCandidates     The linked list of GestureArenaMembers to compete with.
   * @param lynxTouchEvent             The LynxTouchEvent associated with the event.
   * @param bubbleChainCandidates      The linked list of GestureArenaMembers that can bubble the
   *     event, used to stop fling
   */
  public void resolveTouchEvent(MotionEvent event,
      LinkedList<GestureArenaMember> competeChainCandidates, LynxTouchEvent lynxTouchEvent,
      LinkedList<GestureArenaMember> bubbleChainCandidates) {
    switch (event.getActionMasked()) {
      case MotionEvent.ACTION_DOWN: {
        resetCandidatesGestures(competeChainCandidates);
        stopFlingByLastFlingMember(
            lynxTouchEvent, competeChainCandidates, bubbleChainCandidates, event);
        // need to re-compete gesture if developer change state in onBegin callback
        dispatchMotionEventWithSimultaneousAndReCompete(
            mWinner, event.getX(), event.getY(), lynxTouchEvent, competeChainCandidates, event);
        findNextWinnerInBegin(
            lynxTouchEvent, competeChainCandidates, event.getX(), event.getY(), event);

        if (mVelocityTracker == null) {
          mVelocityTracker = VelocityTracker.obtain();
        } else {
          mVelocityTracker.clear();
        }
        mVelocityTracker.addMovement(event);
        break;
      }
      case MotionEvent.ACTION_MOVE:
        // Handle ACTION_MOVE event
        if (mVelocityTracker != null) {
          mVelocityTracker.addMovement(event);
        }
        mWinner = reCompeteByGestures(competeChainCandidates, mWinner);
        if (mWinner == mLastWinner) {
          dispatchMotionEventWithSimultaneous(
              mWinner, event.getX(), event.getY(), lynxTouchEvent, event);
        }
        findNextWinnerInBegin(
            lynxTouchEvent, competeChainCandidates, event.getX(), event.getY(), event);

        break;
      case MotionEvent.ACTION_UP:
      case MotionEvent.ACTION_CANCEL:
        // Handle ACTION_UP and ACTION_CANCEL events
        dispatchMotionEventWithSimultaneousAndReCompete(
            mWinner, event.getX(), event.getY(), null, competeChainCandidates, event);
        int xVelocity = 0;
        int yVelocity = 0;
        if (mVelocityTracker != null) {
          mVelocityTracker.computeCurrentVelocity(1000); // compute what px per second
          xVelocity = LynxUIHelper.px2dip(mContext, mVelocityTracker.getXVelocity());
          yVelocity = LynxUIHelper.px2dip(mContext, mVelocityTracker.getYVelocity());
        }
        if (mWinner != null
            && (Math.abs(xVelocity) > GestureConstants.FLING_SPEED_THRESHOLD
                || Math.abs(yVelocity) > GestureConstants.FLING_SPEED_THRESHOLD)) {
          startGestureFling();
        } else {
          dispatchMotionEventWithSimultaneousAndReCompete(
              mWinner, Float.MIN_VALUE, Float.MIN_VALUE, null, competeChainCandidates, null);
        }
        break;
      default:
        break;
    }
  }

  private void resetCandidatesGestures(List<GestureArenaMember> members) {
    if (members == null) {
      return;
    }
    for (GestureArenaMember member : members) {
      resetGestureHandlerAndSimultaneous(member);
    }
    mDuplicatedMember = null;
  }

  private void failOthersMembersInRaceRelation(
      GestureArenaMember member, int currentGestureId, Set<Integer> simultaneousGestureIds) {
    if (member == null) {
      return;
    }
    // Retrieve the map of gesture handlers associated with the member.
    Map<Integer, BaseGestureHandler> gestureHandler = member.getGestureHandlers();

    // If there are no gesture handlers for the member, there is no need to reset anything, so
    // return.
    if (gestureHandler == null) {
      return;
    }
    // Iterate through each gesture handler associated with the member and fail them exclude
    // simultaneous members.
    for (BaseGestureHandler handler : gestureHandler.values()) {
      if (handler.getGestureDetector().getGestureID() != currentGestureId
          && !simultaneousGestureIds.contains(handler.getGestureDetector().getGestureID())) {
        handler.fail();
      }
    }
  }

  private void stopFlingByLastFlingMember(LynxTouchEvent lynxTouchEvent,
      LinkedList<GestureArenaMember> competeChainCandidates,
      LinkedList<GestureArenaMember> bubbleCandidates, MotionEvent motionEvent) {
    if (bubbleCandidates == null) {
      return;
    }
    for (GestureArenaMember member : bubbleCandidates) {
      if (mWinner != null && mLastFlingTargetId == member.getGestureArenaMemberId()
          || mLastFlingTargetId == 0) {
        mLastFlingTargetId = 0;
        if (!mScroller.isFinished()) {
          dispatchMotionEventWithSimultaneousAndReCompete(
              mWinner, 0, 0, lynxTouchEvent, competeChainCandidates, motionEvent);
          mScroller.abortAnimation();
          // when stop fling, not trigger tap event
          if (mContext != null) {
            mContext.onGestureRecognized();
          }
        }
        break;
      }
    }
  }

  // find next winner in onBegin callback when developer fail in continuous onBegin callback
  private void findNextWinnerInBegin(@Nullable LynxTouchEvent lynxTouchEvent,
      LinkedList<GestureArenaMember> competeChainCandidates, float x, float y, MotionEvent event) {
    for (int i = 0; i < competeChainCandidates.size(); i++) {
      // limit the maximum number of the chain loops to prevent infinite loops
      if (mWinner == mLastWinner || mWinner == null) {
        return;
      }
      updateLastWinner(mWinner);
      updateSimultaneousWinner(mWinner);
      dispatchMotionEventWithSimultaneousAndReCompete(
          mWinner, x, y, lynxTouchEvent, competeChainCandidates, event);
    }
    // if all compete candidates not active, need to end this gesture
    dispatchMotionEventWithSimultaneousAndReCompete(
        mLastWinner, x, y, lynxTouchEvent, competeChainCandidates, event);
  }

  /**
   * Computes the scroll and updates the winner based on the current scroll position.
   *
   * @param competeChainCandidates The linked list of GestureArenaMembers to compete with.
   */
  public void computeScroll(LinkedList<GestureArenaMember> competeChainCandidates) {
    if (mScroller.computeScrollOffset()) {
      int computeX = mScroller.getCurrX();
      int computeY = mScroller.getCurrY();
      int deltaX = computeX - mLastFlingScrollX;
      int deltaY = computeY - mLastFlingScrollY;
      mLastFlingScrollX = computeX;
      mLastFlingScrollY = computeY;
      mLastFlingWinner = reCompeteByGestures(competeChainCandidates, mLastFlingWinner);
      findNextWinnerInBegin(null, competeChainCandidates, 0, 0, null);
      if (mLastFlingWinner != null) {
        mLastFlingTargetId = mLastFlingWinner.getGestureArenaMemberId();
        dispatchMotionEventWithSimultaneous(mLastFlingWinner, deltaX, deltaY, null, null);
        if (mScroller.isFinished()) {
          dispatchMotionEventWithSimultaneousAndReCompete(mLastFlingWinner, Float.MIN_VALUE,
              Float.MIN_VALUE, null, competeChainCandidates, null);
        }
      } else {
        mLastFlingTargetId = 0;
        if (!mScroller.isFinished()) {
          mScroller.abortAnimation();
        }
      }
    }
  }

  /**
   * Updates the simultaneous winner based on the current winner.
   * @param winner
   */
  private void updateSimultaneousWinner(GestureArenaMember winner) {
    Pair<HashSet<GestureArenaMember>, HashSet<Integer>> result =
        mGestureDetectorManager.handleSimultaneousWinner(winner);
    if (result == null) {
      return;
    }
    mSimultaneousWinners = result.first;
    mSimultaneousGestureIds = result.second;
  }

  /**
   * update the last winner
   * @param winner
   */
  private void updateLastWinner(GestureArenaMember winner) {
    if (mLastWinner != winner) {
      mLastWinner = winner;
    }
  }

  /**
   * Determines the new winner among the competitor chain based on the current delta values.
   *
   * @param competitorChainCandidates  The linked list of GestureArenaMembers to compete with.
   * @param current                    The current GestureArenaMember winner.
   * @return                           The new GestureArenaMember winner.
   */
  private GestureArenaMember reCompeteByGestures(
      LinkedList<GestureArenaMember> competitorChainCandidates, GestureArenaMember current) {
    if ((current == null && mLastWinner == null) || competitorChainCandidates == null) {
      return null;
    }
    boolean needReCompeteLastWinner = false;
    if (current == null && mLastWinner != null) {
      needReCompeteLastWinner = true;
      current = mLastWinner;
      resetGestureHandlerAndSimultaneous(mLastWinner);
    }

    int stateCurrent = getCurrentMemberState(current);

    if (stateCurrent <= GestureConstants.LYNX_STATE_ACTIVE) {
      return current;
    } else if (stateCurrent == GestureConstants.LYNX_STATE_END) {
      return null;
    }

    if (needReCompeteLastWinner) {
      return null;
    }

    int index = competitorChainCandidates.indexOf(current);
    int lastIndex = competitorChainCandidates.lastIndexOf(current);
    if (index != lastIndex) {
      if (mDuplicatedMember != null) {
        index = lastIndex;
        resetGestureHandlerAndSimultaneous(mDuplicatedMember);
      } else {
        mDuplicatedMember = competitorChainCandidates.get(index);
      }
    }

    if (index < 0 || index >= competitorChainCandidates.size()) {
      return null;
    }
    // Ignore the same candidates id, such as A -> B -> B -> C, if B is failed, need to judge C
    // Whether the conditions are met reCompete to end of competitor chain
    int currentMemberId = competitorChainCandidates.get(index).getGestureArenaMemberId();
    for (int i = index + 1; i < competitorChainCandidates.size(); i++) {
      GestureArenaMember node = competitorChainCandidates.get(i);
      if (node.getGestureArenaMemberId() == currentMemberId) {
        continue;
      }
      // reset gesture handler to init status before getCurrentMemberState
      if (mDuplicatedMember == node) {
        mDuplicatedMember = null;
      } else {
        resetGestureHandlerAndSimultaneous(node);
      }

      int state = getCurrentMemberState(node);
      if (state <= GestureConstants.LYNX_STATE_ACTIVE) {
        return node;
      } else if (state == GestureConstants.LYNX_STATE_END) {
        return null;
      }
    }
    for (int i = 0; i < index; i++) {
      // reCompete from i to start of competitor chain
      GestureArenaMember node = competitorChainCandidates.get(i);
      if (node.getGestureArenaMemberId() == currentMemberId) {
        continue;
      }

      // reset gesture handler to init status before getCurrentMemberState
      if (mDuplicatedMember == node) {
        mDuplicatedMember = null;
      } else {
        resetGestureHandlerAndSimultaneous(node);
      }

      int state = getCurrentMemberState(node);

      if (state <= GestureConstants.LYNX_STATE_ACTIVE) {
        return node;
      } else if (state == GestureConstants.LYNX_STATE_END) {
        return null;
      }
    }
    return null;
  }

  /**
   * Dispatches touch event to the gesture handlers associated with the current winner in the
   * gesture arena.
   *
   * @param event The motion event to be handled.
   * @param member The current member in the gesture arena.
   */
  private void dispatchMotionEventOnCurrentWinner(@Nullable MotionEvent event,
      @Nullable GestureArenaMember member, @Nullable LynxTouchEvent lynxTouchEvent, float deltaX,
      float deltaY) {
    // If there is no current member, return early as there are no gesture handlers to dispatch the
    // event to.
    if (member == null) {
      return;
    }

    // Retrieve the gesture handlers associated with the current member.
    Map<Integer, BaseGestureHandler> gestureHandler = member.getGestureHandlers();

    // If there are no gesture handlers, return as there is no need to dispatch the event.
    if (gestureHandler == null) {
      return;
    }

    // Iterate through each gesture handler associated with the winner and handle the event.
    for (BaseGestureHandler handler : gestureHandler.values()) {
      handler.handleMotionEvent(event, lynxTouchEvent, deltaX, deltaY);
    }
  }

  /**
   * Checks if the current member is active based on the delta values.
   *
   * @param node    The GestureArenaMember to check.
   */
  private int getCurrentMemberState(@Nullable GestureArenaMember node) {
    if (node == null) {
      return GestureConstants.LYNX_STATE_FAIL;
    }

    Map<Integer, BaseGestureHandler> gestureHandler = node.getGestureHandlers();
    if (gestureHandler == null) {
      return GestureConstants.LYNX_STATE_FAIL;
    }
    int minStatus = -1;
    for (BaseGestureHandler handler : gestureHandler.values()) {
      if (handler.isEnd()) {
        resetGestureHandlerAndSimultaneous(node);
        // if end api invoked, not re-compete gesture to last winner
        mLastWinner = null;
        return GestureConstants.LYNX_STATE_END;
      }
      if (handler.isActive()) {
        failOthersMembersInRaceRelation(
            node, handler.getGestureDetector().getGestureID(), mSimultaneousGestureIds);
        return GestureConstants.LYNX_STATE_ACTIVE;
      }
      if (minStatus < 0) {
        minStatus = handler.getGestureStatus();
      } else if (minStatus > handler.getGestureStatus()) {
        minStatus = handler.getGestureStatus();
      }
    }
    return minStatus;
  }

  private void resetGestureHandlerAndSimultaneous(@Nullable GestureArenaMember member) {
    resetGestureHandler(member);

    if (mSimultaneousWinners != null) {
      for (GestureArenaMember arenaMember : mSimultaneousWinners) {
        resetGestureHandler(arenaMember);
      }
    }
  }

  /**
   * Resets the gesture handlers associated with a given GestureArenaMember.
   *
   * @param member The GestureArenaMember whose gesture handlers need to be reset.
   */
  private void resetGestureHandler(@Nullable GestureArenaMember member) {
    // If the member is null, there are no gesture handlers to reset, so return early.
    if (member == null) {
      return;
    }

    // Retrieve the map of gesture handlers associated with the member.
    Map<Integer, BaseGestureHandler> gestureHandler = member.getGestureHandlers();

    // If there are no gesture handlers for the member, there is no need to reset anything, so
    // return.
    if (gestureHandler == null) {
      return;
    }

    // Iterate through each gesture handler associated with the member and reset them.
    for (BaseGestureHandler handler : gestureHandler.values()) {
      handler.reset();
    }
  }

  /**
   * Recycles the velocity tracker.
   */
  private void recycleVelocityTracker() {
    if (mVelocityTracker != null) {
      try {
        mVelocityTracker.recycle();
      } catch (IllegalStateException e) {
        LLog.e(TAG, e.toString());
      }
      mVelocityTracker = null;
    }
  }

  private void startGestureFling() {
    if (mWinner == null || mVelocityTracker == null) {
      return;
    }
    mLastFlingWinner = mWinner;
    mScroller.fling(mLastFlingWinner.getMemberScrollX(),
        mLastFlingWinner.getMemberScrollY(), // start position
        (int) -mVelocityTracker.getXVelocity(),
        (int) -mVelocityTracker.getYVelocity(), // fling velocity
        GestureConstants.MIN_SCROLL, GestureConstants.MAX_SCROLL, GestureConstants.MIN_SCROLL,
        GestureConstants.MAX_SCROLL, // min and max scrollY
        0, 0); // overscroll
    mLastFlingWinner.onInvalidate();
    mLastFlingScrollX = mLastFlingWinner.getMemberScrollX();
    mLastFlingScrollY = mLastFlingWinner.getMemberScrollY();
  }

  /**
   * Dispatches the active event to gesture handlers based on the event type, gesture type mask, and
   * delta values with simultaneous
   *
   * @param winner             The current GestureArenaMember winner.
   * @param x                  The horizontal delta value.
   * @param y                  The vertical delta value.
   */
  private void dispatchMotionEventWithSimultaneous(@Nullable GestureArenaMember winner, float x,
      float y, @Nullable LynxTouchEvent lynxTouchEvent, MotionEvent event) {
    dispatchMotionEventWithSimultaneousAndReCompete(winner, x, y, lynxTouchEvent, null, event);
  }

  /**
   * Dispatches the active event to gesture handlers based on the event type, gesture type mask, and
   * delta values with simultaneous, re-compete gesture to current winner
   *
   * @param winner             The current GestureArenaMember winner.
   * @param x                  The horizontal delta / position value.
   * @param y                  The vertical delta / position value.
   * @param lynxTouchEvent     The touchEvent info
   * @param competeChainCandidates The compete chain candidates
   */
  private void dispatchMotionEventWithSimultaneousAndReCompete(@Nullable GestureArenaMember winner,
      float x, float y, @Nullable LynxTouchEvent lynxTouchEvent,
      @Nullable LinkedList<GestureArenaMember> competeChainCandidates, MotionEvent motionEvent) {
    if (winner == null) {
      return;
    }

    dispatchMotionEventOnCurrentWinner(motionEvent, winner, lynxTouchEvent, x, y);
    if (mSimultaneousWinners != null) {
      for (GestureArenaMember member : mSimultaneousWinners) {
        dispatchMotionEventOnCurrentWinner(motionEvent, member, lynxTouchEvent, x, y);
      }
    }
    if (competeChainCandidates != null) {
      mWinner = reCompeteByGestures(competeChainCandidates, mWinner);
    }
  }

  /**
   * Handles the gesture detector state changes and performs actions accordingly.
   *
   * @param member     The GestureArenaMember associated with the state change.
   * @param gestureId  The gesture ID of the gesture detector.
   * @param state      The state of the gesture detector.
   */
  public void handleGestureDetectorState(
      @Nullable GestureArenaMember member, int gestureId, int state) {
    if (member == null) {
      return;
    }
    BaseGestureHandler handler = getGestureHandlerById(member, gestureId);
    switch (state) {
      case LynxNewGestureDelegate.STATE_ACTIVE:
        break;
      case LynxNewGestureDelegate.STATE_FAIL:
        if (handler != null) {
          handler.fail();
        }
        break;
      case LynxNewGestureDelegate.STATE_END:
        if (handler != null) {
          handler.end();
        }
        break;
      default:
        break;
    }
  }

  /**
   * Retrieves the gesture handler by gesture ID associated with the GestureArenaMember.
   *
   * @param member     The GestureArenaMember associated with the gesture handler.
   * @param gestureId  The gesture ID of the gesture handler.
   * @return           The BaseGestureHandler associated with the gesture ID, or null if not found.
   */
  @Nullable
  public BaseGestureHandler getGestureHandlerById(GestureArenaMember member, int gestureId) {
    Map<Integer, BaseGestureHandler> handlerMap = member.getGestureHandlers();
    if (handlerMap == null) {
      return null;
    }
    for (BaseGestureHandler handler : handlerMap.values()) {
      if (handler.getGestureDetector().getGestureID() == gestureId) {
        return handler;
      }
    }
    return null;
  }

  /**
   * Dispatches the bubble touch event to gesture handlers based on the event type and touch event.
   *
   * @param type                The event type.
   * @param touchEvent          The LynxTouchEvent associated with the event.
   * @param bubbleCandidate     The linked list of GestureArenaMembers that can bubble the event.
   * @param winner              The current GestureArenaMember winner.
   */
  public void dispatchBubbleTouchEvent(String type, LynxTouchEvent touchEvent,
      LinkedList<GestureArenaMember> bubbleCandidate, GestureArenaMember winner) {
    if (winner == null) {
      return;
    }

    if (!LynxTouchEvent.EVENT_TOUCH_START.equals(type)
        && !LynxTouchEvent.EVENT_TOUCH_MOVE.equals(type)
        && !LynxTouchEvent.EVENT_TOUCH_CANCEL.equals(type)
        && !LynxTouchEvent.EVENT_TOUCH_END.equals(type)) {
      return;
    }

    for (int i = 0; i < bubbleCandidate.size(); i++) {
      Map<Integer, BaseGestureHandler> gestureHandler = bubbleCandidate.get(i).getGestureHandlers();
      if (gestureHandler == null) {
        continue;
      }
      for (BaseGestureHandler handler : gestureHandler.values()) {
        switch (type) {
          case LynxTouchEvent.EVENT_TOUCH_START:
            handler.onTouchesDown(touchEvent);
            break;
          case LynxTouchEvent.EVENT_TOUCH_MOVE:
            handler.onTouchesMove(touchEvent);
            break;
          case LynxTouchEvent.EVENT_TOUCH_END:
            handler.onTouchesUp(touchEvent);
            break;
          case LynxTouchEvent.EVENT_TOUCH_CANCEL:
            handler.onTouchesCancel(touchEvent);
            break;
        }
      }
    }
  }

  /**
   * Cleans up resources when the GestureHandlerTrigger is destroyed.
   */
  public void onDestroy() {
    recycleVelocityTracker();
    mContext = null;
    if (mSimultaneousWinners != null) {
      mSimultaneousWinners.clear();
    }
  }
}
