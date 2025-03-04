// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file may have been modified by Lynx.
 * All changes are commented starting with Lynx
 * Summary: remove class or function which is not public
 *          remove logic of double-tap and fling
 *          add setLongPressTimeout
 */

package com.lynx.tasm.behavior;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

/**
 * Detects various gestures and events using the supplied {@link MotionEvent}s.
 * The {@link OnGestureListener} callback will notify users when a particular
 * motion event has occurred. This class should only be used with {@link MotionEvent}s
 * reported via touch (don't use for trackball events).
 *
 * To use this class:
 * <ul>
 *  <li>Create an instance of the {@code GestureRecognizer} for your {@link View}
 *  <li>In the {@link View#onTouchEvent(MotionEvent)} method ensure you call
 *          {@link #onTouchEvent(MotionEvent)}. The methods defined in your callback
 *          will be executed when the events occur.
 * </ul>
 */
// rename GestureDetector to GestureRecognizer.
public class GestureRecognizer {
  /**
   * The listener that is used to notify when gestures occur.
   * If you want to listen for all the different gestures then implement
   * this interface. If you only want to listen for a subset it might
   * be easier to extend {@link SimpleOnGestureListener}.
   */
  public interface OnGestureListener {
    /**
     * Notified when a tap occurs with the down {@link MotionEvent}
     * that triggered it. This will be triggered immediately for
     * every down event. All other events should be preceded by this.
     *
     * @param e The down motion event.
     */
    boolean onDown(MotionEvent e);

    /**
     * The user has performed a down {@link MotionEvent} and not performed
     * a move or up yet. This event is commonly used to provide visual
     * feedback to the user to let them know that their action has been
     * recognized i.e. highlight an element.
     *
     * @param e The down motion event
     */
    void onShowPress(MotionEvent e);

    /**
     * Notified when a tap occurs with the up {@link MotionEvent}
     * that triggered it.
     *
     * @param e The up motion event that completed the first tap
     * @return true if the event is consumed, else false
     */
    boolean onSingleTapUp(MotionEvent e);

    /**
     * Notified when a scroll occurs with the initial on down {@link MotionEvent} and the
     * current move {@link MotionEvent}. The distance in x and y is also supplied for
     * convenience.
     *
     * @param e1 The first down motion event that started the scrolling.
     * @param e2 The move motion event that triggered the current onScroll.
     * @param distanceX The distance along the X axis that has been scrolled since the last
     *              call to onScroll. This is NOT the distance between {@code e1}
     *              and {@code e2}.
     * @param distanceY The distance along the Y axis that has been scrolled since the last
     *              call to onScroll. This is NOT the distance between {@code e1}
     *              and {@code e2}.
     * @return true if the event is consumed, else false
     */
    boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY);

    /**
     * Notified when a long press occurs with the initial on down {@link MotionEvent}
     * that trigged it.
     *
     * @param e The initial on down motion event that started the longpress.
     */
    void onLongPress(MotionEvent e);

    /**
     * Notified of a fling event when it occurs with the initial on down {@link MotionEvent}
     * and the matching up {@link MotionEvent}. The calculated velocity is supplied along
     * the x and y axis in pixels per second.
     *
     * @param e1 The first down motion event that started the fling.
     * @param e2 The move motion event that triggered the current onFling.
     * @param velocityX The velocity of this fling measured in pixels per second
     *              along the x axis.
     * @param velocityY The velocity of this fling measured in pixels per second
     *              along the y axis.
     * @return true if the event is consumed, else false
     */
    boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY);
  }

  /**
   * The listener that is used to notify when a double-tap or a confirmed
   * single-tap occur.
   */
  public interface OnDoubleTapListener {
    /**
     * Notified when a single-tap occurs.
     * <p>
     * Unlike {@link OnGestureListener#onSingleTapUp(MotionEvent)}, this
     * will only be called after the detector is confident that the user's
     * first tap is not followed by a second tap leading to a double-tap
     * gesture.
     *
     * @param e The down motion event of the single-tap.
     * @return true if the event is consumed, else false
     */
    boolean onSingleTapConfirmed(MotionEvent e);

    /**
     * Notified when a double-tap occurs.
     *
     * @param e The down motion event of the first tap of the double-tap.
     * @return true if the event is consumed, else false
     */
    boolean onDoubleTap(MotionEvent e);

    /**
     * Notified when an event within a double-tap gesture occurs, including
     * the down, move, and up events.
     *
     * @param e The motion event that occurred during the double-tap gesture.
     * @return true if the event is consumed, else false
     */
    boolean onDoubleTapEvent(MotionEvent e);
  }

  /**
   * A convenience class to extend when you only want to listen for a subset
   * of all the gestures. This implements all methods in the
   * {@link OnGestureListener} and {@link OnDoubleTapListener} but does
   * nothing and return {@code false} for all applicable methods.
   */
  public static class SimpleOnGestureListener implements OnGestureListener, OnDoubleTapListener {
    public boolean onSingleTapUp(MotionEvent e) {
      return false;
    }

    public void onLongPress(MotionEvent e) {}

    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
      return false;
    }

    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
      return false;
    }

    public void onShowPress(MotionEvent e) {}

    public boolean onDown(MotionEvent e) {
      return false;
    }

    public boolean onDoubleTap(MotionEvent e) {
      return false;
    }

    public boolean onDoubleTapEvent(MotionEvent e) {
      return false;
    }

    public boolean onSingleTapConfirmed(MotionEvent e) {
      return false;
    }
  }

  private int mTouchSlopSquare;
  /* double-tap and fling-velocity is not needed
   * private int mDoubleTapTouchSlopSquare;
   * private int mDoubleTapSlopSquare;
   * private int mMinimumFlingVelocity;
   * private int mMaximumFlingVelocity;
   */

  private static int LONGPRESS_TIMEOUT = ViewConfiguration.getLongPressTimeout();
  private static final int TAP_TIMEOUT = ViewConfiguration.getTapTimeout();
  // double-tap and fling-velocity is not needed
  // private static final int DOUBLE_TAP_TIMEOUT = ViewConfiguration.getDoubleTapTimeout();

  // constants for Message.what used by GestureHandler below
  private static final int SHOW_PRESS = 1;
  private static final int LONG_PRESS = 2;
  private static final int TAP = 3;

  private final Handler mHandler;
  private final OnGestureListener mListener;
  private OnDoubleTapListener mDoubleTapListener;

  private boolean mStillDown;
  private boolean mInLongPress;
  private boolean mAlwaysInTapRegion;
  private boolean mAlwaysInBiggerTapRegion;

  private MotionEvent mCurrentDownEvent;
  private MotionEvent mPreviousUpEvent;

  /**
   * True when the user is still touching for the second tap (down, move, and
   * up events). Can only be true if there is a double tap listener attached.
   */
  private boolean mIsDoubleTapping;

  private float mLastMotionY;
  private float mLastMotionX;

  private boolean mIsLongpressEnabled;

  /**
   * True if we are at a target API level of >= Froyo or the developer can
   * explicitly set it. If true, input events with > 1 pointer will be ignored
   * so we can work side by side with multitouch gesture detectors.
   */
  private boolean mIgnoreMultitouch;

  /**
   * Determines speed during touch scrolling
   */
  // mVelocityTracker is used by fling, not needed.
  // private VelocityTracker mVelocityTracker;

  /**
   * Consistency verifier for debugging purposes.
   */

  /* InputEventConsistencyVerifier is not a public class, so remove it.
   * private final InputEventConsistencyVerifier mInputEventConsistencyVerifier =
   *     InputEventConsistencyVerifier.isInstrumentationEnabled()
   *     ? new InputEventConsistencyVerifier(this, 0)
   *     : null;
   */

  private class GestureHandler extends Handler {
    GestureHandler() {
      super();
    }

    GestureHandler(Handler handler) {
      super(handler.getLooper());
    }

    @Override
    public void handleMessage(Message msg) {
      switch (msg.what) {
        case SHOW_PRESS:
          mListener.onShowPress(mCurrentDownEvent);
          break;

        case LONG_PRESS:
          dispatchLongPress();
          break;

        case TAP:
          // If the user's finger is still down, do not count it as a tap
          if (mDoubleTapListener != null && !mStillDown) {
            mDoubleTapListener.onSingleTapConfirmed(mCurrentDownEvent);
          }
          break;

        default:
          throw new RuntimeException("Unknown message " + msg); // never
      }
    }
  }

  /**
   * Creates a GestureRecognizer with the supplied listener.
   * This variant of the constructor should be used from a non-UI thread
   * (as it allows specifying the Handler).
   *
   * @param listener the listener invoked for all the callbacks, this must
   * not be null.
   * @param handler the handler to use
   *
   * @throws NullPointerException if either {@code listener} or
   * {@code handler} is null.
   *
   * @deprecated Use {@link #GestureRecognizer(android.content.Context,
   *      android.view.GestureRecognizer.OnGestureListener, android.os.Handler)} instead.
   */
  @Deprecated
  public GestureRecognizer(OnGestureListener listener, Handler handler) {
    this(null, listener, handler);
  }

  /**
   * Creates a GestureRecognizer with the supplied listener.
   * You may only use this constructor from a UI thread (this is the usual situation).
   * @see android.os.Handler#Handler()
   *
   * @param listener the listener invoked for all the callbacks, this must
   * not be null.
   *
   * @throws NullPointerException if {@code listener} is null.
   *
   * @deprecated Use {@link #GestureRecognizer(android.content.Context,
   *      android.view.GestureRecognizer.OnGestureListener)} instead.
   */
  @Deprecated
  public GestureRecognizer(OnGestureListener listener) {
    this(null, listener, null);
  }

  /**
   * Creates a GestureRecognizer with the supplied listener.
   * You may only use this constructor from a UI thread (this is the usual situation).
   * @see android.os.Handler#Handler()
   *
   * @param context the application's context
   * @param listener the listener invoked for all the callbacks, this must
   * not be null.
   *
   * @throws NullPointerException if {@code listener} is null.
   */
  public GestureRecognizer(Context context, OnGestureListener listener) {
    this(context, listener, null);
  }

  /**
   * Creates a GestureRecognizer with the supplied listener.
   * You may only use this constructor from a UI thread (this is the usual situation).
   * @see android.os.Handler#Handler()
   *
   * @param context the application's context
   * @param listener the listener invoked for all the callbacks, this must
   * not be null.
   * @param handler the handler to use
   *
   * @throws NullPointerException if {@code listener} is null.
   */
  public GestureRecognizer(Context context, OnGestureListener listener, Handler handler) {
    this(context, listener, handler,
        context != null
            && context.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.FROYO);
  }

  /**
   * Creates a GestureRecognizer with the supplied listener.
   * You may only use this constructor from a UI thread (this is the usual situation).
   * @see android.os.Handler#Handler()
   *
   * @param context the application's context
   * @param listener the listener invoked for all the callbacks, this must
   * not be null.
   * @param handler the handler to use
   * @param ignoreMultitouch whether events involving more than one pointer should
   * be ignored.
   *
   * @throws NullPointerException if {@code listener} is null.
   */
  public GestureRecognizer(
      Context context, OnGestureListener listener, Handler handler, boolean ignoreMultitouch) {
    if (handler != null) {
      mHandler = new GestureHandler(handler);
    } else {
      mHandler = new GestureHandler();
    }
    mListener = listener;
    if (listener instanceof OnDoubleTapListener) {
      setOnDoubleTapListener((OnDoubleTapListener) listener);
    }
    init(context, ignoreMultitouch);
  }

  private void init(Context context, boolean ignoreMultitouch) {
    if (mListener == null) {
      throw new NullPointerException("OnGestureListener must not be null");
    }
    mIsLongpressEnabled = true;
    mIgnoreMultitouch = ignoreMultitouch;
    updateTouchSlop(context);
  }

  /**
   * When replacing the Context, we can call this method to update the configuration to
   * ensure accuracy.
   * @param context
   */
  public void updateTouchSlop(Context context) {
    // Fallback to support pre-donuts releases
    int touchSlop, doubleTapSlop, doubleTapTouchSlop;
    if (context == null) {
      // noinspection deprecation
      touchSlop = ViewConfiguration.getTouchSlop();
      /* getDoubleTapSlop() is not public, and double-tap, fling is not needed.
       * doubleTapTouchSlop = touchSlop; // Hack rather than adding a hiden method for this
       * doubleTapSlop = ViewConfiguration.getDoubleTapSlop();
       * // noinspection deprecation
       * mMinimumFlingVelocity = ViewConfiguration.getMinimumFlingVelocity();
       * mMaximumFlingVelocity = ViewConfiguration.getMaximumFlingVelocity();
       */
    } else {
      final ViewConfiguration configuration = ViewConfiguration.get(context);
      touchSlop = configuration.getScaledTouchSlop();
      /* getScaledDoubleTapTouchSlop() is not public, and double-tap, fling is not
       * needed.
       * doubleTapTouchSlop = configuration.getScaledDoubleTapTouchSlop();
       * doubleTapSlop = configuration.getScaledDoubleTapSlop();
       * mMinimumFlingVelocity = configuration.getScaledMinimumFlingVelocity();
       * mMaximumFlingVelocity = configuration.getScaledMaximumFlingVelocity();
       */
    }
    mTouchSlopSquare = touchSlop * touchSlop;
    /* double-tap not needed
     * mDoubleTapTouchSlopSquare = doubleTapTouchSlop * doubleTapTouchSlop;
     * mDoubleTapSlopSquare = doubleTapSlop * doubleTapSlop;
     */
  }

  /**
   * Sets the listener which will be called for double-tap and related
   * gestures.
   *
   * @param onDoubleTapListener the listener invoked for all the callbacks, or
   *        null to stop listening for double-tap gestures.
   */
  public void setOnDoubleTapListener(OnDoubleTapListener onDoubleTapListener) {
    mDoubleTapListener = onDoubleTapListener;
  }

  // add setLongPressTimeout to modify longPressTimeout
  public void setLongPressTimeout(int longPressTimeout) {
    LONGPRESS_TIMEOUT = longPressTimeout;
  }

  /**
   * Set whether longpress is enabled, if this is enabled when a user
   * presses and holds down you get a longpress event and nothing further.
   * If it's disabled the user can press and hold down and then later
   * moved their finger and you will get scroll events. By default
   * longpress is enabled.
   *
   * @param isLongpressEnabled whether longpress should be enabled.
   */
  public void setIsLongpressEnabled(boolean isLongpressEnabled) {
    mIsLongpressEnabled = isLongpressEnabled;
  }

  /**
   * @return true if longpress is enabled, else false.
   */
  public boolean isLongpressEnabled() {
    return mIsLongpressEnabled;
  }

  /**
   * Analyzes the given motion event and if applicable triggers the
   * appropriate callbacks on the {@link OnGestureListener} supplied.
   *
   * @param ev The current motion event.
   * @return true if the {@link OnGestureListener} consumed the event,
   *              else false.
   */
  public boolean onTouchEvent(MotionEvent ev) {
    /* mInputEventConsistencyVerifier is removed
     * if (mInputEventConsistencyVerifier != null) {
     *   mInputEventConsistencyVerifier.onTouchEvent(ev, 0);
     * }
     */
    final int action = ev.getAction();
    final float y = ev.getY();
    final float x = ev.getX();

    /* Used by fling
     * if (mVelocityTracker == null) {
     *   mVelocityTracker = VelocityTracker.obtain();
     * }
     * mVelocityTracker.addMovement(ev);
     */

    boolean handled = false;

    switch (action & MotionEvent.ACTION_MASK) {
      case MotionEvent.ACTION_POINTER_DOWN:
        if (mIgnoreMultitouch) {
          // Multitouch event - abort.
          cancel();
        }
        break;

      case MotionEvent.ACTION_POINTER_UP:
        // Ending a multitouch gesture and going back to 1 finger
        if (mIgnoreMultitouch && ev.getPointerCount() == 2) {
          int index = (((action & MotionEvent.ACTION_POINTER_INDEX_MASK)
                           >> MotionEvent.ACTION_POINTER_INDEX_SHIFT)
                          == 0)
              ? 1
              : 0;
          mLastMotionX = ev.getX(index);
          mLastMotionY = ev.getY(index);
          /* fling is removed
           * mVelocityTracker.recycle();
           * mVelocityTracker = VelocityTracker.obtain();
           */
        }
        break;

      case MotionEvent.ACTION_DOWN:
        /* double tap is removed.
         * if (mDoubleTapListener != null) {
         *   boolean hadTapMessage = mHandler.hasMessages(TAP);
         *   if (hadTapMessage)
         *     mHandler.removeMessages(TAP);
         *   if ((mCurrentDownEvent != null) && (mPreviousUpEvent != null) && hadTapMessage
         *       && isConsideredDoubleTap(mCurrentDownEvent, mPreviousUpEvent, ev)) {
         *     // This is a second tap
         *     mIsDoubleTapping = true;
         *     // Give a callback with the first tap of the double-tap
         *     handled |= mDoubleTapListener.onDoubleTap(mCurrentDownEvent);
         *     // Give a callback with down event of the double-tap
         *     handled |= mDoubleTapListener.onDoubleTapEvent(ev);
         *   } else {
         *     // This is a first tap
         *     mHandler.sendEmptyMessageDelayed(TAP, DOUBLE_TAP_TIMEOUT);
         *   }
         * }
         */

        mLastMotionX = x;
        mLastMotionY = y;
        if (mCurrentDownEvent != null) {
          mCurrentDownEvent.recycle();
        }
        mCurrentDownEvent = MotionEvent.obtain(ev);
        mAlwaysInTapRegion = true;
        mAlwaysInBiggerTapRegion = true;
        mStillDown = true;
        mInLongPress = false;

        if (mIsLongpressEnabled) {
          mHandler.removeMessages(LONG_PRESS);
          mHandler.sendEmptyMessageAtTime(
              LONG_PRESS, mCurrentDownEvent.getDownTime() + TAP_TIMEOUT + LONGPRESS_TIMEOUT);
        }
        mHandler.sendEmptyMessageAtTime(SHOW_PRESS, mCurrentDownEvent.getDownTime() + TAP_TIMEOUT);
        handled |= mListener.onDown(ev);
        break;

      case MotionEvent.ACTION_MOVE:
        if (mInLongPress || (mIgnoreMultitouch && ev.getPointerCount() > 1)) {
          break;
        }
        final float scrollX = mLastMotionX - x;
        final float scrollY = mLastMotionY - y;
        if (mIsDoubleTapping) {
          // Give the move events of the double-tap
          // double-tap is not used
          // handled |= mDoubleTapListener.onDoubleTapEvent(ev);
        } else if (mAlwaysInTapRegion) {
          final int deltaX = (int) (x - mCurrentDownEvent.getX());
          final int deltaY = (int) (y - mCurrentDownEvent.getY());
          int distance = (deltaX * deltaX) + (deltaY * deltaY);
          if (distance > mTouchSlopSquare) {
            handled = mListener.onScroll(mCurrentDownEvent, ev, scrollX, scrollY);
            mLastMotionX = x;
            mLastMotionY = y;
            mAlwaysInTapRegion = false;
            mHandler.removeMessages(TAP);
            mHandler.removeMessages(SHOW_PRESS);
            mHandler.removeMessages(LONG_PRESS);
          }
          /* double-tap is not used
           * if (distance > mDoubleTapTouchSlopSquare) {
           *   mAlwaysInBiggerTapRegion = false;
           * }
           */
        } else if ((Math.abs(scrollX) >= 1) || (Math.abs(scrollY) >= 1)) {
          handled = mListener.onScroll(mCurrentDownEvent, ev, scrollX, scrollY);
          mLastMotionX = x;
          mLastMotionY = y;
        }
        break;

      case MotionEvent.ACTION_UP:
        mStillDown = false;
        MotionEvent currentUpEvent = MotionEvent.obtain(ev);
        if (mIsDoubleTapping) {
          // Finally, give the up event of the double-tap
          // double-tap is not used
          // handled |= mDoubleTapListener.onDoubleTapEvent(ev);
        } else if (mInLongPress) {
          mHandler.removeMessages(TAP);
          mInLongPress = false;
        } else if (mAlwaysInTapRegion) {
          handled = mListener.onSingleTapUp(ev);
        } else {
          /* fling is not used.
           * // A fling must travel the minimum tap distance
           * final VelocityTracker velocityTracker = mVelocityTracker;
           * velocityTracker.computeCurrentVelocity(1000, mMaximumFlingVelocity);
           * final float velocityY = velocityTracker.getYVelocity();
           * final float velocityX = velocityTracker.getXVelocity();
           *
           * if ((Math.abs(velocityY) > mMinimumFlingVelocity)
           *     || (Math.abs(velocityX) > mMinimumFlingVelocity)) {
           *   handled = mListener.onFling(mCurrentDownEvent, ev, velocityX, velocityY);
           * }
           */
        }
        if (mPreviousUpEvent != null) {
          mPreviousUpEvent.recycle();
        }
        // Hold the event we obtained above - listeners may have changed the original.
        mPreviousUpEvent = currentUpEvent;
        /* fling and double-tap is not used.
         * if (mVelocityTracker != null) {
         *   // This may have been cleared when we called out to the
         *   // application above.
         *   mVelocityTracker.recycle();
         *   mVelocityTracker = null;
         * }
         * mIsDoubleTapping = false;
         */
        mHandler.removeMessages(SHOW_PRESS);
        mHandler.removeMessages(LONG_PRESS);
        break;

      case MotionEvent.ACTION_CANCEL:
        cancel();
        break;
    }

    /* mInputEventConsistencyVerifier is removed
     * if (!handled && mInputEventConsistencyVerifier != null) {
     *   mInputEventConsistencyVerifier.onUnhandledEvent(ev, 0);
     * }
     */
    return handled;
  }

  private void cancel() {
    mHandler.removeMessages(SHOW_PRESS);
    mHandler.removeMessages(LONG_PRESS);
    mHandler.removeMessages(TAP);
    /* fling and double-tap is not used.
     * mVelocityTracker.recycle();
     * mVelocityTracker = null;
     * mIsDoubleTapping = false;
     */
    mStillDown = false;
    mAlwaysInTapRegion = false;
    mAlwaysInBiggerTapRegion = false;
    if (mInLongPress) {
      mInLongPress = false;
    }
  }

  /* double-tap is not used.
   * private boolean isConsideredDoubleTap(
   *     MotionEvent firstDown, MotionEvent firstUp, MotionEvent secondDown) {
   *   if (!mAlwaysInBiggerTapRegion) {
   *     return false;
   *   }
   *
   *   if (secondDown.getEventTime() - firstUp.getEventTime() > DOUBLE_TAP_TIMEOUT) {
   *     return false;
   *   }
   *
   *   int deltaX = (int) firstDown.getX() - (int) secondDown.getX();
   *   int deltaY = (int) firstDown.getY() - (int) secondDown.getY();
   *   return (deltaX * deltaX + deltaY * deltaY < mDoubleTapSlopSquare);
   * }
   */

  private void dispatchLongPress() {
    mHandler.removeMessages(TAP);
    mInLongPress = true;
    mListener.onLongPress(mCurrentDownEvent);
  }
}
