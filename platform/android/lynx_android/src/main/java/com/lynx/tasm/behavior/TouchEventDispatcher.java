// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_LONG_PRESS;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_CANCEL;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_END;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_MOVE;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_START;

import android.content.Context;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.event.LynxEventDetail.EVENT_TYPE;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.event.LynxTouchEvent.Point;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map.Entry;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.json.JSONException;
import org.json.JSONObject;

public class TouchEventDispatcher {
  private static class EventTargetDetail {
    private final EventTarget mActiveUI;
    private final PointF mDownPoint;
    private PointF mPrePoint;

    public EventTargetDetail(EventTarget activeUI, float x, float y) {
      mActiveUI = activeUI;
      mDownPoint = new PointF(x, y);
      mPrePoint = new PointF(x, y);
    }

    public EventTarget getUI() {
      return mActiveUI;
    }

    public PointF getDownPoint() {
      return mDownPoint;
    }

    public PointF getPrePoint() {
      return mPrePoint;
    }

    public void setPrePoint(PointF prePoint) {
      mPrePoint = prePoint;
    }
  }

  private LynxUIOwner mUIOwner;
  private GestureRecognizer mDetector;
  private EventTarget mActiveUI;
  // pointId -> Target, a pointId corresponds to a finger.
  private HashMap<Integer, EventTargetDetail> mActiveUIMap;
  private HashMap<Integer, EventTargetBase> mActiveTargetMap;
  private LinkedList<EventTarget> mActiveUIList;
  private EventTarget mFocusedUI;
  private LinkedList<EventTarget> mActiveClickList;
  // Manages the gesture arenas for handling touch events
  private GestureArenaManager mGestureArenaManager;

  private EventTarget.EnableStatus mConsumeSlideEvent = EventTarget.EnableStatus.Undefined;
  private boolean mCanConsumeSlideEvent = false;
  private PointF mDownPoint;
  private float mTapSlop;

  private boolean mTouchMoved;
  private boolean mTouchMoving;
  private float mMoveSlop;
  private boolean mShouldCheckMove;
  private boolean mGestureRecognized;
  private boolean mTouchOutSide;
  private PointF mPrePoint;
  private final HashSet<Integer> mGestureRecognizedUISet;
  private final HashSet<Integer> mPropsChangedUISet;
  private boolean mHasTouchPseudo;
  private boolean mHasMultiTouch;
  private boolean mEnableMultiTouch;
  private long mTimestamp = 0;

  private Point mTargetPoint;
  // record first index of multi touch event
  private LynxTouchEvent mFirstLynxTouchEvent;
  private EventTarget mPreTarget;
  private String mPreTargetInlineCSSText;

  private static final String TAG = "LynxTouchEventDispatcher";

  public TouchEventDispatcher(LynxUIOwner owner) {
    mUIOwner = owner;
    Listener mListener = new Listener();
    mDetector = new GestureRecognizer(
        mUIOwner.getContext(), mListener, new Handler(Looper.getMainLooper()));
    mActiveUIList = new LinkedList<>();
    mActiveClickList = new LinkedList<>();
    mDownPoint = new PointF(Float.MIN_VALUE, Float.MIN_VALUE);
    // In template_binary_reader.cc, if template's page config doesn't contain tapSlop, let
    // tapSlop's the default value be 50px. Otherwise, tapSlop's value is set by the FE developer.
    // Normally, TouchEventDispatcher's setTapSlop function will be called, and mTapSlop will be
    // PixelUtils.dipToPx(tapSlop's value), which is greater than 0 . However, when LynxOverlayView
    // creates TouchEventDispatcher, the TouchEventDispatcher's setTapSlop function is not called.
    // Thus, mTapSlop in LynxOverlayView's TouchEventDispatcher will be 0, which means that the tap
    // gesture is recognize only if there is no ACTION_MOVE between the ACTION_DOWN and ACTION_UP.
    // But on some android phones, there will be ACTION_MOVE between ACTION_DWON and ACTION_UP even
    // if there is no movement between the finger down and up. Which leads to the bug that on these
    // phones, the tap event cannot be triggered in the overlay. To solve this bug, let mTapSlop's
    // default value be PixelUtils.dipToPx(50). And another fix will be provided. With the future
    // fix, when LynxOverlayView creates TouchEventDispatcher, the TouchEventDispatcher's setTapSlop
    // and other set-methods will be called. And E2E test case will be added in future.
    // TODO(songshourui.null): Add E2E test case.
    mTapSlop = PixelUtils.dipToPx(50);
    mMoveSlop = 0;
    mShouldCheckMove = true;
    mGestureRecognizedUISet = new HashSet<>();
    mPropsChangedUISet = new HashSet<>();
    mHasTouchPseudo = false;
    mHasMultiTouch = false;
    mEnableMultiTouch = false;
    mActiveUIMap = new HashMap<>();
    mActiveTargetMap = new HashMap<>();
    if (owner != null) {
      mGestureArenaManager = owner.getGestureArenaManager();
    }
  }

  /**
   * Support Context replacement
   * @param context
   */
  public void attachContext(Context context) {
    if (mDetector != null) {
      mDetector.updateTouchSlop(context);
    }
  }

  // TODO(hexionghui): Delete this, use the same method with paramenter instead.
  public void onGestureRecognized() {
    mGestureRecognized = true;
    if (!mActiveUIList.isEmpty()) {
      // if gesture is recognized, tap will not be triggered, so remove the active state of all
      // active UI.
      deactivatePseudoState(LynxTouchEvent.kPseudoStateActive);
    }
  }

  public void setGestureArenaManager(GestureArenaManager manager) {
    mGestureArenaManager = manager;
  }

  public void onGestureRecognized(LynxBaseUI ui) {
    if (mGestureRecognizedUISet != null) {
      mGestureRecognizedUISet.add(ui.getSign());
    }
  }

  public void onPropsChanged(LynxBaseUI ui) {
    if (mPropsChangedUISet != null) {
      mPropsChangedUISet.add(ui.getSign());
    }
  }

  public boolean isTouchMoving() {
    return mTouchMoving;
  }

  public void setHasTouchPseudo(boolean value) {
    // When disable fiber arch, setHasTouchPseudo will be exec twice.
    // Normally, it will exec setHasTouchPseudo in onPageConfigDecoded first.
    // In case not following this order in the future and exec setHasTouchPseudo in updateEventInfo
    // first, let mHasTouchPseudo = mHasTouchPseudo || value;
    mHasTouchPseudo = mHasTouchPseudo || value;
  }

  public void setEnableMultiTouch(boolean value) {
    mEnableMultiTouch = value;
  }

  private boolean requestNativeDisallowIntercept(boolean flag) {
    if (mUIOwner.getRootUI() == null || mUIOwner.getRootUI().getView().getParent() == null) {
      LLog.e(TAG, "requestNativeDisallowIntercept failed, root ui or root ui'parent is null.");
      return false;
    }
    mUIOwner.getRootUI().getView().getParent().requestDisallowInterceptTouchEvent(flag);
    return flag;
  }

  public boolean consumeSlideEvent(MotionEvent ev) {
    switch (ev.getAction()) {
      case MotionEvent.ACTION_DOWN: {
        // When the finger is pressed, set mConsumeSlideEvent to Undefined.
        mConsumeSlideEvent = EventTarget.EnableStatus.Undefined;
        mCanConsumeSlideEvent = false;
        if (mActiveUI != null) {
          EventTarget target = mActiveUI;
          while (target != null && target.parent() != target) {
            // Only when there is a node on the responder chain that has consume-slide-event set,
            // can the detection logic of consumeSlideEvent continue to be executed.
            if (target.hasConsumeSlideEventAngles()) {
              mCanConsumeSlideEvent = true;
            }
            target = target.parent();
          }
        }

        if (mCanConsumeSlideEvent) {
          // When executing consumeSlideEvent, if MotionEvent.ACTION_DOWN is encountered,
          // immediately execute requestNativeDisallowIntercept(true) and return false. According to
          // the previous logic, if break is encountered at this point, the function will return
          // false and also call requestNativeDisallowIntercept(false). This may cause the ancestors
          // of LynxView to immediately consume the event, resulting in a bad case for
          // consume-slide-event.
          requestNativeDisallowIntercept(mCanConsumeSlideEvent);
        }
        return false;
      }
      case MotionEvent.ACTION_MOVE: {
        if (!mCanConsumeSlideEvent) {
          return false;
        }

        // Calculate the distance and angle of finger movement. To avoid the dead zone in angle
        // calculation during the initial stage of finger movement, when distanceX and distanceY are
        // small, return true first to make LynxView consume the event. When the finger moves beyond
        // a certain threshold, calculate the angle to determine whether to consume the slide event.
        float distanceX = ev.getX() - mDownPoint.x;
        float distanceY = ev.getY() - mDownPoint.y;
        float threshold = 10;
        if (Math.abs(distanceX) <= PixelUtils.dipToPx(threshold)
            && Math.abs(distanceY) <= PixelUtils.dipToPx(threshold)) {
          requestNativeDisallowIntercept(true);
          return false;
        }
        // If mConsumeSlideEvent != Undefined, then judge whether mConsumeSlideEvent == Enable. If
        // it is Enable, return true to consume subsequent events. Otherwise, return false to not
        // consume subsequent events.
        if (mConsumeSlideEvent != EventTarget.EnableStatus.Undefined) {
          break;
        }
        // When the finger moves, set the default value of mConsumeSlideEvent to Disable, then
        // calculate the sliding angle. If it needs to consume the angle, set mConsumeSlideEvent to
        // Enable. Otherwise, let mConsumeSlideEvent remain Disable.
        mConsumeSlideEvent = EventTarget.EnableStatus.Disable;

        // Use atan2(y, x) * 180 / PI to calculate the angle.
        float semicircleAngle = 180;
        double angle = Math.atan2(distanceY, distanceX) * semicircleAngle / Math.PI;
        EventTarget target = mActiveUI;
        while (target != null && target.parent() != target) {
          if (target.consumeSlideEvent((float) angle)) {
            mConsumeSlideEvent = EventTarget.EnableStatus.Enable;
            break;
          }
          target = target.parent();
        }
        break;
      }
      default:
        break;
    }
    boolean res = (mConsumeSlideEvent == EventTarget.EnableStatus.Enable);
    return requestNativeDisallowIntercept(res);
  }

  public boolean blockNativeEvent(MotionEvent ev) {
    if (mActiveUI == null) {
      return false;
    }
    EventTarget target = mActiveUI;
    while (target != null && target.parent() != target) {
      if (target.blockNativeEvent(ev)) {
        return true;
      }
      target = target.parent();
    }
    return false;
  }

  public boolean eventThrough() {
    if (mActiveUI == null) {
      return false;
    }
    return mActiveUI.eventThrough();
  }

  public void setTapSlop(float tapSlop) {
    mTapSlop = tapSlop;
  }

  private int checkCanRespondTapOrClick(EventTarget ui, HashSet set) {
    if (ui == null) {
      return 0;
    }
    if (set == null || set.isEmpty()) {
      return -1;
    }
    int res = -1;
    EventTarget parent = ui;
    while (parent != null && parent.parent() != parent) {
      if (set.contains(parent.getSign())) {
        res = parent.getSign();
        break;
      }
      parent = parent.parent();
    }
    return res;
  }

  // Specify whether the tap/click event can be triggered when the ui slides.
  // At this time, the ui slide is triggered by a gesture.
  private int canRespondTapOrClick(EventTarget ui) {
    return checkCanRespondTapOrClick(ui, mGestureRecognizedUISet);
  }

  // Specify whether the tap/click event can be triggered when the ui slides.
  // At this time, the ui slide is triggered by the front-end modifying the
  // animation or layout attributes.
  private int canRespondTapOrClickWhenUISlideWithProps(EventTarget ui) {
    return checkCanRespondTapOrClick(ui, mPropsChangedUISet);
  }

  private void initTouchEnv(MotionEvent ev) {
    mTouchMoved = false;
    mTouchMoving = false;
    mShouldCheckMove = true;
    mDownPoint = new PointF(ev.getX(), ev.getY());
    mGestureRecognized = false;
    mPrePoint = new PointF(ev.getX(), ev.getY());
    mGestureRecognizedUISet.clear();
    mPropsChangedUISet.clear();
    mHasMultiTouch = false;
    mActiveUIMap.clear();
    mActiveTargetMap.clear();
  }

  private void initClickEnv() {
    mActiveClickList.clear();
    if (mActiveUI == null) {
      return;
    }
    EventTarget tempUi = mActiveUI;
    while (tempUi != null) {
      mActiveClickList.push(tempUi);
      tempUi = tempUi.parent();
    }
    while (!mActiveClickList.isEmpty()
        && (mActiveClickList.getLast().getEvents() == null
            || !mActiveClickList.getLast().getEvents().containsKey(EVENT_CLICK))) {
      mActiveClickList.removeLast();
    }
    for (EventTarget target : mActiveClickList) {
      target.onResponseChain();
    }
    if (mActiveClickList.isEmpty()) {
      mTouchOutSide = true;
    } else {
      mTouchOutSide = false;
    }
  }

  private boolean onTouchMove(MotionEvent ev, int index) {
    boolean res = false;
    boolean firstTouchMoved = false;
    EventTargetDetail detail = mActiveUIMap.get(ev.getPointerId(index));
    if (detail != null) {
      PointF prePoint = detail.getPrePoint();
      PointF downPoint = detail.getDownPoint();
      // TODO(hexionghui): Fix the problem: In single-finger mode, the pressing position of the
      // first finger and the current position of the last finger are always used to determine
      // whether to move.
      if (!mEnableMultiTouch) {
        downPoint = mDownPoint;
      }
      // TODO(hexionghui): Align with the tapslop logic on the iOS side: xDiff^2 + yDiff^2 >
      // tapslop^2
      if (prePoint.x != ev.getX(index) || prePoint.y != ev.getY(index)) {
        if (Math.abs(downPoint.x - ev.getX(index)) > mTapSlop
            || Math.abs(downPoint.y - ev.getY(index)) > mTapSlop) {
          mTouchMoved = true;
          if (ev.getPointerId(index) == 0) {
            firstTouchMoved = true;
          }
        }
        prePoint.x = ev.getX(index);
        prePoint.y = ev.getY(index);
        mPrePoint = prePoint;
        res = true;
      }

      detail.setPrePoint(prePoint);
    }

    if (res) {
      // For better performance, the following code is executed only when the touch position
      // changes. Because the response chain does not change when the touch position does not
      // change, thus, there is no need to exec the following code.
      EventTarget target = findUI(ev, index, mUIOwner.getRootUI());
      mTouchOutSide = mGestureRecognized
          || (!mActiveClickList.isEmpty() && canRespondTapOrClick(mActiveClickList.getLast()) != -1)
          || mTouchOutSide || eventOutSideActiveList(ev, target);
      // Only when the first finger is moved, :active will be disabled.
      if (!mActiveUIList.isEmpty() && (!mEnableMultiTouch && mTouchMoved)
          || (mEnableMultiTouch && firstTouchMoved) || canRespondTapOrClick(mActiveUI) != -1
          || canRespondTapOrClickWhenUISlideWithProps(mActiveUI) != -1) {
        // If touchMoved, tap will not be triggered, so remove the active state of all active UI.
        deactivatePseudoState(LynxTouchEvent.kPseudoStateActive);
      }
    }

    return res;
  }

  private void fireClick(MotionEvent e) {
    // For the click event, it only support single finger.
    if (mActiveClickList.isEmpty() || mActiveClickList.getLast() == null) {
      return;
    }
    int slideTargetSign = canRespondTapOrClick(mActiveClickList.getLast());
    int propsTargetSign = canRespondTapOrClickWhenUISlideWithProps(mActiveClickList.getLast());
    if ((!mEnableMultiTouch || !mHasMultiTouch) && !mTouchOutSide && !mGestureRecognized
        && !mActiveClickList.isEmpty() && mActiveClickList.getLast() != null
        && slideTargetSign == -1 && propsTargetSign == -1) {
      dispatchEvent(mActiveClickList.getLast(), EVENT_CLICK, e);
    }
  }

  private void fireTap(MotionEvent e) {
    // For the tap event, it only support single finger.
    int slideTargetSign = canRespondTapOrClick(mActiveUI);
    int propsTargetSign = canRespondTapOrClickWhenUISlideWithProps(mActiveUI);
    if ((!mEnableMultiTouch || !mHasMultiTouch) && !mGestureRecognized && !mTouchMoved
        && slideTargetSign == -1 && propsTargetSign == -1) {
      if (LynxEnv.inst().isHighlightTouchEnabled()) {
        showMessageOnConsole(
            TAG + ": fire tap for target " + mActiveUI.getSign(), LogBoxLogLevel.Info.ordinal());
      }
      dispatchEvent(mActiveUI, EVENT_TAP, e);
    } else {
      if (LynxEnv.inst().isHighlightTouchEnabled()) {
        showMessageOnConsole(TAG + ": tap failed due to [gesture] " + mGestureRecognized
                + ", [move] " + mTouchMoved + ", [slide] " + slideTargetSign + ", [props] "
                + propsTargetSign,
            LogBoxLogLevel.Warn.ordinal());
      }
      LLog.i(TAG,
          "tap failed:" + mGestureRecognized + " " + mTouchMoved + " " + slideTargetSign + " "
              + propsTargetSign);
    }
  }

  private void resetEnv() {
    for (EventTarget target : mActiveClickList) {
      target.offResponseChain();
    }
    mActiveUIList.clear();
    mActiveClickList.clear();
    mGestureRecognizedUISet.clear();
    mPropsChangedUISet.clear();
    mTouchMoved = false;
    mTouchMoving = false;
    mShouldCheckMove = true;
    mHasMultiTouch = false;
    mActiveUIMap.clear();
    mActiveTargetMap.clear();
    mPreTarget = mActiveUI;
  }

  public void destroy() {}
  // When ActionDown, generate event target response chain. And traversed the event target response
  // chain to make the target's touch state pseudo-class take effect.
  private void onActionDown(MotionEvent ev) {
    // TODO(songshourui.null): for fiber Arch, need to enable mHasTouchPseudo by default
    if (eventEmitter() == null) {
      return;
    }
    EventTarget target = mActiveUI;
    while (target != null) {
      mActiveUIList.push(target);
      if (!target.enableTouchPseudoPropagation()) {
        break;
      }
      target = target.parent();
    }
    for (int i = 0; i < mActiveUIList.size(); ++i) {
      EventTarget ui = mActiveUIList.get(i);
      if (mHasTouchPseudo) {
        // Only execute this function if mHasTouchPseudo is true
        eventEmitter().onPseudoStatusChanged(
            ui.getSign(), LynxTouchEvent.kPseudoStateNone, LynxTouchEvent.kPseudoStateActive);
      }
      ui.onPseudoStatusChanged(LynxTouchEvent.kPseudoStateNone, LynxTouchEvent.kPseudoStateActive);
    }
  }

  // When ActionMove, the touched event target may change. Disable the touch pseudo class for
  // targets not on the response chain.
  void onActionMove(MotionEvent ev, EventTarget target) {
    if (mUIOwner == null || mUIOwner.getRootUI() == null || mActiveUIList.isEmpty()
        || eventEmitter() == null) {
      return;
    }
    EventTarget newUI = target;
    LinkedList<EventTarget> list = new LinkedList<>();
    while (newUI != null) {
      list.push(newUI);
      if (!newUI.enableTouchPseudoPropagation()) {
        break;
      }
      newUI = newUI.parent();
    }

    int index = -1;
    for (int i = 0; i < mActiveUIList.size() && i < list.size(); ++i) {
      EventTarget preTarget = mActiveUIList.get(i);
      EventTarget nowTarget = list.get(i);
      if (preTarget.getSign() != nowTarget.getSign()) {
        break;
      }
      index = i;
    }

    for (int i = mActiveUIList.size() - 1; i >= index + 1; --i) {
      EventTarget ui = mActiveUIList.get(i);
      if (mHasTouchPseudo) {
        // Only execute this function if mHasTouchPseudo is true
        eventEmitter().onPseudoStatusChanged(
            ui.getSign(), LynxTouchEvent.kPseudoStateActive, LynxTouchEvent.kPseudoStateNone);
      }
      ui.onPseudoStatusChanged(LynxTouchEvent.kPseudoStateActive, LynxTouchEvent.kPseudoStateNone);
      mActiveUIList.remove(i);
    }
  }

  // When ActionUp, the touched event target may change. Disable the touch pseudo class for all
  // targets.
  private void onActionUpOrCancel(MotionEvent ev) {
    deactivatePseudoState(LynxTouchEvent.kPseudoStateAll);
  }

  private void deactivatePseudoState(int state) {
    if (eventEmitter() == null) {
      return;
    }
    for (EventTarget ui : mActiveUIList) {
      if (mHasTouchPseudo) {
        // Only execute this function if mHasTouchPseudo is true
        eventEmitter().onPseudoStatusChanged(
            ui.getSign(), ui.getPseudoStatus(), ui.getPseudoStatus() & ~state);
      }
      ui.onPseudoStatusChanged(ui.getPseudoStatus(), ui.getPseudoStatus() & ~state);
    }
  }

  // dispatch event for touch* .
  // TODO(hexionghui): Merge two dispatchEvent interfaces into one.
  private void dispatchEvent(String eventName, MotionEvent ev, JavaOnlyMap map) {
    mFirstLynxTouchEvent = initialFirstLynxTouchEvent(mActiveUI, eventName, ev);
    mFirstLynxTouchEvent.setMotionEvent(ev);
    LynxTouchEvent event = new LynxTouchEvent(eventName, map);
    event.setMotionEvent(ev);
    event.setActiveTargetMap(mActiveTargetMap);
    event.setTimestamp(mTimestamp);
    if (mGestureArenaManager != null) {
      mGestureArenaManager.dispatchBubbleTouchEvent(eventName, mFirstLynxTouchEvent);
    }
    eventEmitter().sendMultiTouchEvent(event);
  }

  // dispatch event for tap, click, longpress.
  private void dispatchEvent(EventTarget target, String eventName, MotionEvent ev) {
    mTargetPoint = convertToViewPoint(mActiveUI, new Point(ev.getX(0), ev.getY(0)));
    LynxTouchEvent.Point pagePoint = new LynxTouchEvent.Point(ev.getX(0), ev.getY(0));
    PointF point = LynxUIHelper.convertPointFromUIToScreen(
        mUIOwner.getRootUI(), new PointF(pagePoint.getX(), pagePoint.getY()));
    LynxTouchEvent.Point clientPoint = new Point(point.x, point.y);
    mFirstLynxTouchEvent =
        new LynxTouchEvent(target.getSign(), eventName, clientPoint, pagePoint, mTargetPoint);
    mFirstLynxTouchEvent.setMotionEvent(ev);
    mFirstLynxTouchEvent.setTarget(mActiveUI);
    mFirstLynxTouchEvent.setTimestamp(mTimestamp);

    if (EVENT_TOUCH_START.equals(eventName)) {
      inspectHitTarget();
      if (LynxEnv.inst().isHighlightTouchEnabled()) {
        showMessageOnConsole(TAG + ": hit the target with sign = " + target.getSign(),
            LogBoxLogLevel.Info.ordinal());
      }
    }

    if (eventEmitter() == null) {
      LLog.i(TAG, "dispatchEvent failed since eventEmitter() null");
      return;
    }

    // dispatch bubble touch event to gesture arena
    if (mGestureArenaManager != null) {
      mGestureArenaManager.dispatchBubbleTouchEvent(eventName, mFirstLynxTouchEvent);
    }
    eventEmitter().sendTouchEvent(mFirstLynxTouchEvent);
  }

  /**
   * Used to output logs to the console of DevTool. This function is effective only when DevTool is
   * connected.
   * @param msg Information related to event processing.
   * @param level The log level.
   */
  // TODO(hexionghui): Postpone string concatenation and encapsulate the method in LynxDevtool.
  private void showMessageOnConsole(String msg, int level) {
    if (mUIOwner.getRootUI() == null || mUIOwner.getRootUI().getBodyView() == null) {
      return;
    }
    LynxBaseInspectorOwner inspectorOwner =
        ((LynxView) mUIOwner.getRootUI().getBodyView()).getBaseInspectorOwner();
    if (inspectorOwner == null) {
      return;
    }
    inspectorOwner.showMessageOnConsole(msg, level);
  }

  /**
   * Used to highlight the nodes that are actually touched. This function takes effect only when
   * LynxDevtool and HighlightTouch are turned on.
   */
  private void inspectHitTarget() {
    if (!LynxEnv.inst().isHighlightTouchEnabled() || mActiveUI == null
        || mUIOwner.getRootUI() == null || mUIOwner.getRootUI().getBodyView() == null) {
      return;
    }
    LynxBaseInspectorOwner inspectorOwner =
        ((LynxView) mUIOwner.getRootUI().getBodyView()).getBaseInspectorOwner();
    if (inspectorOwner == null) {
      return;
    }
    try {
      JSONObject json = new JSONObject();
      json.put("id", 1);
      json.put("method", "DOM.setAttributesAsText");
      if (mPreTarget != null && mPreTargetInlineCSSText != null) {
        JSONObject params = new JSONObject();
        params.put("nodeId", mPreTarget.getSign());
        params.put("text", String.format("style=\"%s\"", mPreTargetInlineCSSText));
        params.put("name", "style");
        json.put("params", params);
        inspectorOwner.invokeCDPFromSDK(json.toString(), new CDPResultCallback() {
          @Override
          public void onResult(String result) {
            LLog.i(TAG, "DOM.setAttributesAsText:" + result);
          }
        });
      }

      json.put("method", "CSS.getInlineStylesForNode");
      JSONObject params = new JSONObject();
      params.put("nodeId", mActiveUI.getSign());
      json.put("params", params);
      inspectorOwner.invokeCDPFromSDK(json.toString(), new CDPResultCallback() {
        @Override
        public void onResult(String result) {
          try {
            setAttributeByInvokeCDP(json, params, inspectorOwner, result);
          } catch (JSONException e) {
            LLog.e(TAG, "setAttributeByInvokeCDP error:" + e.toString());
          }
        }
      });

    } catch (Exception e) {
      LLog.e(TAG, "inspectHitTarget json generate error");
    }
  }

  private void setAttributeByInvokeCDP(JSONObject json, JSONObject params,
      LynxBaseInspectorOwner inspectorOwner, String inlineStyle) throws JSONException {
    String regex = "\"cssText\"\\s*:\\s*\"([^\"]*)\"";
    Pattern pattern = Pattern.compile(regex);
    Matcher matcher = pattern.matcher(inlineStyle);
    if (matcher.find()) {
      mPreTargetInlineCSSText = matcher.group(1);
    }

    String cssText = (mPreTargetInlineCSSText != null ? mPreTargetInlineCSSText : "")
        + "background-color:#9CC4E6;border-width:2px;border-color:red;";
    json.put("method", "DOM.setAttributesAsText");
    params.put("text", String.format("style=\"%s\"", cssText));
    params.put("name", "style");
    json.put("params", params);
    inspectorOwner.invokeCDPFromSDK(json.toString(), new CDPResultCallback() {
      @Override
      public void onResult(String result) {
        LLog.i(TAG, "DOM.setAttributesAsText:" + result);
      }
    });
  }

  private boolean shouldTriggerMove(MotionEvent ev) {
    if (mShouldCheckMove) {
      float distanceX = ev.getX(0) - mDownPoint.x;
      float distanceY = ev.getY(0) - mDownPoint.y;
      if (Math.abs(distanceX) > mMoveSlop || Math.abs(distanceY) > mMoveSlop) {
        mShouldCheckMove = false;
        return true;
      }
      return false;
    }
    return true;
  }

  public boolean onTouchEvent(MotionEvent ev, UIGroup rootUi) {
    mTimestamp = System.currentTimeMillis();
    if (ev.getActionMasked() == MotionEvent.ACTION_DOWN) {
      mActiveUI = findUI(ev, 0, rootUi);
      if (mActiveUI != null && mActiveUI.eventThrough()) {
        return false;
      }
      initTouchEnv(ev);
      initClickEnv();
      mActiveUIMap.put(
          ev.getPointerId(0), new EventTargetDetail(mActiveUI, ev.getX(0), ev.getY(0)));
      mActiveTargetMap.put(mActiveUI.getSign(), mActiveUI);

      // set the active ui to gesture arena
      if (mGestureArenaManager != null) {
        mGestureArenaManager.setActiveUIToArenaAtDownEvent(mActiveUI);
      }
      int longPressDuration = ViewConfiguration.getLongPressTimeout();
      if (mUIOwner.getContext().getLongPressDuration() >= 0) {
        longPressDuration = mUIOwner.getContext().getLongPressDuration();
      }
      mDetector.setLongPressTimeout(longPressDuration);

      if (mEnableMultiTouch) {
        JavaOnlyMap map = new JavaOnlyMap();
        addMap(map, ev, 0);
        dispatchEvent(EVENT_TOUCH_START, ev, map);
      } else {
        dispatchEvent(mActiveUI, EVENT_TOUCH_START, ev);
      }

      // TODO(hexionghui): For the :active logic, it should only support single finger. But on the
      // Android side, touching two fingers at the same time will trigger onTouchEvent twice,
      // causing the :active on the Android side to also take effect when two fingers touch it at
      // the same time.
      onActionDown(ev);
    } else if (ev.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN) {
      mHasMultiTouch = true;
      // mActiveUIMap should always be updated rather than controlled by mEnableMultiTouch to
      // prevent NPE errors in onTouchMove.
      int index = ev.getActionIndex();
      EventTarget active_target = findUI(ev, index, rootUi);
      mActiveUIMap.put(ev.getPointerId(index),
          new EventTargetDetail(active_target, ev.getX(index), ev.getY(index)));
      mActiveTargetMap.put(active_target.getSign(), active_target);
      if (mEnableMultiTouch) {
        JavaOnlyMap map = new JavaOnlyMap();
        addMap(map, ev, index);
        dispatchEvent(EVENT_TOUCH_START, ev, map);
      }
    } else {
      if (mActiveUI != null && !mActiveUIMap.isEmpty()) {
        if (mActiveUI.eventThrough()) {
          return false;
        }
        switch (ev.getActionMasked()) {
          case MotionEvent.ACTION_MOVE:
            if (shouldTriggerMove(ev)) {
              mTouchMoving = true;
              if (mEnableMultiTouch) {
                JavaOnlyMap map = new JavaOnlyMap();
                for (int index = 0; index < ev.getPointerCount(); ++index) {
                  if (onTouchMove(ev, index)) {
                    addMap(map, ev, index);
                  }
                }
                dispatchEvent(EVENT_TOUCH_MOVE, ev, map);
              } else {
                if (onTouchMove(ev, 0)) {
                  dispatchEvent(mActiveUI, EVENT_TOUCH_MOVE, ev);
                }
              }
            }
            break;

          case MotionEvent.ACTION_POINTER_UP:
            int index = ev.getActionIndex();
            if (mEnableMultiTouch) {
              if (ev.getPointerId(index) == 0) {
                onActionUpOrCancel(ev);
              }
              JavaOnlyMap map = new JavaOnlyMap();
              addMap(map, ev, index);
              dispatchEvent(EVENT_TOUCH_END, ev, map);
            }
            mActiveUIMap.remove(ev.getPointerId(index));
            break;
          case MotionEvent.ACTION_UP:
            if (mActiveUI != null && !mActiveUI.ignoreFocus() && !mGestureRecognized
                && canRespondTapOrClick(mActiveUI) == -1
                && canRespondTapOrClickWhenUISlideWithProps(mActiveUI) == -1) {
              EventTarget prevActiveUI = mFocusedUI;
              mFocusedUI = mActiveUI;
              if (mActiveUI != prevActiveUI) {
                if (mActiveUI != null && mActiveUI.isFocusable()) {
                  mActiveUI.onFocusChanged(
                      true, prevActiveUI != null && prevActiveUI.isFocusable());
                }
                if (prevActiveUI != null && prevActiveUI.isFocusable()) {
                  prevActiveUI.onFocusChanged(false, mActiveUI != null && mActiveUI.isFocusable());
                }
              }
            }

            if (mEnableMultiTouch) {
              JavaOnlyMap map = new JavaOnlyMap();
              addMap(map, ev, 0);
              dispatchEvent(EVENT_TOUCH_END, ev, map);
            } else {
              dispatchEvent(mActiveUI, EVENT_TOUCH_END, ev);
            }

            // TODO(hexionghui): Fix the problem: In single-finger mode, only when the last finger
            // is lifted, :active will be disabled.
            onActionUpOrCancel(ev);
            fireClick(ev);
            // TODO(hexionghui): The tap event should be triggered by the first finger being lifted,
            // not the last finger being lifted.
            fireTap(ev);
            resetEnv();
            break;
          case MotionEvent.ACTION_CANCEL:
            if (mEnableMultiTouch) {
              JavaOnlyMap map = new JavaOnlyMap();
              for (EventTargetDetail detail : mActiveUIMap.values()) {
                addMap(map, ev, 0);
              }
              dispatchEvent(EVENT_TOUCH_CANCEL, ev, map);
            } else {
              dispatchEvent(mActiveUI, EVENT_TOUCH_CANCEL, ev);
            }

            onActionUpOrCancel(ev);
            resetEnv();
            break;
          default:
            break;
        }
      }
    }

    if (mActiveUI != null && mActiveUI.eventThrough()) {
      return false;
    }

    if (mActiveUI != null) {
      mActiveUI.dispatchTouch(ev);
    }
    mDetector.onTouchEvent(ev);

    // dispatch touch event to gesture arena
    if (mGestureArenaManager != null) {
      mGestureArenaManager.dispatchTouchEventToArena(ev, mFirstLynxTouchEvent);
    }

    return true;
  }

  private void addMap(JavaOnlyMap map, MotionEvent ev, int index) {
    EventTargetDetail detail = mActiveUIMap.get(ev.getPointerId(index));
    if (detail == null) {
      return;
    }

    EventTarget activeUI = detail.getUI();
    String sign = String.valueOf(activeUI.getSign());
    ReadableArray events = map.getArray(sign);
    JavaOnlyArray event = new JavaOnlyArray();

    LynxTouchEvent.Point pagePoint = new Point(ev.getX(index), ev.getY(index));
    LynxTouchEvent.Point targetPoint = pagePoint;
    if (activeUI instanceof LynxBaseUI) {
      targetPoint = convertToViewPoint(activeUI, pagePoint);
    }
    PointF point = LynxUIHelper.convertPointFromUIToScreen(
        mUIOwner.getRootUI(), new PointF(pagePoint.getX(), pagePoint.getY()));
    LynxTouchEvent.Point clientPoint = new Point(point.x, point.y);

    event.add(ev.getPointerId(index));
    event.add(clientPoint.getX());
    event.add(clientPoint.getY());
    event.add(pagePoint.getX());
    event.add(pagePoint.getY());
    event.add(targetPoint.getX());
    event.add(targetPoint.getY());
    if (events != null) {
      events.asArrayList().add(event);
    } else {
      JavaOnlyArray newEvents = new JavaOnlyArray();
      newEvents.add(event);
      map.putArray(sign, newEvents);
    }
  }

  public void setFocusedUI(LynxBaseUI focusedUI) {
    mFocusedUI = focusedUI;
  }

  private LynxTouchEvent.Point convertToViewPoint(
      EventTarget activeUI, LynxTouchEvent.Point pagePoint) {
    if (activeUI instanceof LynxBaseUI) {
      LynxBaseUI ui = (LynxBaseUI) activeUI;
      RectF viewRect = LynxUIHelper.convertRectFromUIToRootUI(
          ui, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      LynxTouchEvent.Point viewPoint =
          new Point(pagePoint.getX() - viewRect.left, pagePoint.getY() - viewRect.top);
      return viewPoint;
    }
    return pagePoint;
  }

  private LynxTouchEvent initialFirstLynxTouchEvent(
      EventTarget activeUI, String type, MotionEvent ev) {
    LynxTouchEvent.Point pagePoint = new LynxTouchEvent.Point(ev.getX(), ev.getY());
    PointF point = LynxUIHelper.convertPointFromUIToScreen(
        mUIOwner.getRootUI(), new PointF(pagePoint.getX(), pagePoint.getY()));
    LynxTouchEvent.Point clientPoint = new Point(point.x, point.y);
    return new LynxTouchEvent(activeUI.getSign(), type, clientPoint, pagePoint, mTargetPoint);
  }

  private EventEmitter eventEmitter() {
    return mUIOwner.getContext().getEventEmitter();
  }

  private EventTarget findUI(MotionEvent ev, int pointerIndex, UIGroup rootUi) {
    if (rootUi == null) {
      rootUi = mUIOwner.getRootUI();
    }
    return rootUi.hitTest(ev.getX(pointerIndex), ev.getY(pointerIndex));
  }

  private boolean eventOutSideActiveList(MotionEvent ev, EventTarget target) {
    if (mUIOwner == null || mUIOwner.getRootUI() == null) {
      return true;
    }
    EventTarget newUI = target;
    LinkedList<EventTarget> list = new LinkedList<>();
    while (newUI != null) {
      list.push(newUI);
      newUI = newUI.parent();
    }
    if (list.size() < mActiveClickList.size()) {
      return true;
    }
    for (int i = 0; i < mActiveClickList.size(); ++i) {
      EventTarget ui = mActiveClickList.get(i);
      if (ui == null || ui != list.get(i)) {
        return true;
      }
    }
    return false;
  }

  private class Listener extends GestureRecognizer.SimpleOnGestureListener {
    @Override
    public void onLongPress(MotionEvent e) {
      if ((!mEnableMultiTouch || !mHasMultiTouch) && mActiveUI != null
          && canRespondTapOrClick(mActiveUI) == -1
          && canRespondTapOrClickWhenUISlideWithProps(mActiveUI) == -1) {
        dispatchEvent(mActiveUI, EVENT_LONG_PRESS, e);
      }
      super.onLongPress(e);
    }
  }

  public void reset() {
    mActiveUI = null;
    mFocusedUI = null;
    mActiveClickList.clear();
  }
}
