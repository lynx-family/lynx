// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator

import android.content.Context
import android.view.MotionEvent
import android.view.VelocityTracker
import android.view.View
import android.view.ViewConfiguration
import android.view.ViewGroup
import android.widget.OverScroller
import androidx.core.view.NestedScrollingChild2
import androidx.core.view.NestedScrollingChildHelper
import androidx.core.view.ViewCompat
import com.google.android.material.appbar.AppBarLayout
import com.google.android.material.appbar.CollapsingToolbarLayout
import com.lynx.tasm.base.LLog
import java.lang.reflect.Method
import kotlin.math.abs

class ScrollCoordinatorLayout(
  context: Context,
  private val coordinatorHost: LynxUIScrollCoordinator,
) : ScrollCoordinatorToolbarLayout<ScrollCoordinatorAppBarLayout>(context), NestedScrollingChild2 {
  companion object {
    const val TAG = "ScrollCoordinatorLayout"
  }

  private var lastXIntercept = 0f
  private var lastYIntercept = 0f
  private var interceptTouchEvent = false
  private var nestedScrollAsChild = false
  private var isScrolling = false
  private var downEventHandled = true
  private var consumeGesture: Boolean? = null
  private var interceptGesture: Boolean? = null
  private val childHelper = NestedScrollingChildHelper(this)
  private var velocityTracker: VelocityTracker? = null
  private val scroller = OverScroller(context)
  private var lastTouchY = 0
  private val scrollOffset = IntArray(2)
  private val scrollConsumed = IntArray(2)
  private val nestedOffsets = IntArray(2)
  private var dispatchOffsetUpdatesMethod: Method? = null

  open fun getCollapsingToolbarLayout(): CollapsingToolbarLayout {
    return getCollapsingToolbar()
  }

  override fun addSlotView(slotView: View) {
    val layoutParams =
      LayoutParams(
        ViewGroup.LayoutParams(
          ViewGroup.LayoutParams.MATCH_PARENT,
          ViewGroup.LayoutParams.MATCH_PARENT,
        ),
      )
    layoutParams.behavior = AppBarLayout.ScrollingViewBehavior()
    slotView.layoutParams = layoutParams
    addView(slotView)
  }

  private fun scrollSelf(dy: Int): Int {
    val params = appBarLayoutView.layoutParams as? LayoutParams ?: return 0
    val behavior = params.behavior as? AppBarLayout.Behavior ?: return 0
    val currentOffset = behavior.topAndBottomOffset
    val totalScrollRange = appBarLayoutView.totalScrollRange
    val desiredOffset = currentOffset - dy
    val clampedOffset = desiredOffset.coerceIn(-totalScrollRange, 0)
    if (clampedOffset == currentOffset) {
      return 0
    }

    behavior.topAndBottomOffset = clampedOffset
    dispatchOffsetUpdates()
    return currentOffset - clampedOffset
  }

  private fun dispatchOffsetUpdates() {
    try {
      if (dispatchOffsetUpdatesMethod == null) {
        dispatchOffsetUpdatesMethod =
          appBarLayoutView::class.java.superclass.superclass.getDeclaredMethod(
            "dispatchOffsetUpdates",
            Int::class.java,
          )
        dispatchOffsetUpdatesMethod?.isAccessible = true
      }
      dispatchOffsetUpdatesMethod?.invoke(appBarLayoutView, appBarLayoutView.topAndBottomOffset)
    } catch (e: Exception) {
      LLog.e(TAG, "dispatchOffsetUpdate fail, offset is" + appBarLayoutView.topAndBottomOffset)
      dispatchOffsetUpdatesMethod = null
    }
  }

  private fun recycleVelocityTracker() {
    velocityTracker?.recycle()
    velocityTracker = null
  }

  private fun shouldSkipGestureConsumption(event: MotionEvent?): Boolean {
    return coordinatorHost.isEnableNewGesture &&
      (consumeGesture != null && !consumeGesture!!) &&
      event?.actionMasked != MotionEvent.ACTION_DOWN
  }

  private fun isNativeGestureExcluded(): Boolean {
    return coordinatorHost.isEnableNewGesture && !coordinatorHost.includeNativeGesture
  }

  private fun hasExplicitInterceptGesture(): Boolean {
    return coordinatorHost.isEnableNewGesture && interceptGesture != null
  }

  private fun shouldInterceptGesture(): Boolean {
    return hasExplicitInterceptGesture() && interceptGesture == true
  }

  override fun onTouchEvent(event: MotionEvent?): Boolean {
    if (shouldSkipGestureConsumption(event)) {
      return false
    }

    if (hasExplicitInterceptGesture()) {
      when (event?.actionMasked) {
        MotionEvent.ACTION_DOWN -> parent.requestDisallowInterceptTouchEvent(true)
        MotionEvent.ACTION_MOVE -> {
          parent.requestDisallowInterceptTouchEvent(interceptTouchEvent)
          var result = interceptGesture == true
          if (interceptGesture != true) {
            result = super.onTouchEvent(event)
          }
          return result
        }
        MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> interceptGesture = null
      }
    }

    if (!scrollEnabledFlag) {
      return false
    }

    val handledByParent = if (!nestedScrollAsChild) super.onTouchEvent(event) else true
    val motionEvent = event ?: return true

    if (nestedScrollAsChild) {
      if (velocityTracker == null) {
        velocityTracker = VelocityTracker.obtain()
      }
      velocityTracker?.addMovement(motionEvent)
    }

    val y = (motionEvent.y + 0.5f).toInt()
    when (motionEvent.actionMasked) {
      MotionEvent.ACTION_DOWN -> {
        if (!scroller.isFinished) {
          scroller.forceFinished(true)
        }
        lastTouchY = y
        isScrolling = false
        nestedOffsets[0] = 0
        nestedOffsets[1] = 0
        if (nestedScrollAsChild) {
          if (velocityTracker == null) {
            velocityTracker = VelocityTracker.obtain()
          } else {
            velocityTracker?.clear()
          }
          startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, ViewCompat.TYPE_TOUCH)
        }
      }
      MotionEvent.ACTION_MOVE -> {
        var dy = lastTouchY - y
        if (!isScrolling && abs(dy) > ViewConfiguration.get(context).scaledTouchSlop) {
          isScrolling = true
          if (nestedScrollAsChild) {
            parent.requestDisallowInterceptTouchEvent(true)
          }
        }

        if (nestedScrollAsChild && isScrolling) {
          if (dispatchNestedPreScroll(0, dy, scrollConsumed, scrollOffset, ViewCompat.TYPE_TOUCH)) {
            dy -= scrollConsumed[1]
            nestedOffsets[0] += scrollOffset[0]
            nestedOffsets[1] += scrollOffset[1]
          }

          lastTouchY = y - scrollOffset[1]
          val consumedByChild = scrollSelf(dy)
          val remaining = dy - consumedByChild

          if (dispatchNestedScroll(
              0,
              consumedByChild,
              0,
              remaining,
              scrollOffset,
              ViewCompat.TYPE_TOUCH,
            )
          ) {
            lastTouchY -= scrollOffset[1]
            nestedOffsets[0] += scrollOffset[0]
            nestedOffsets[1] += scrollOffset[1]
          }
        }
      }
      MotionEvent.ACTION_UP -> {
        if (nestedScrollAsChild && isScrolling) {
          velocityTracker?.computeCurrentVelocity(1000)
          val velocityY = velocityTracker?.yVelocity ?: 0f
          val minimumVelocity = ViewConfiguration.get(context).scaledMinimumFlingVelocity
          if (abs(velocityY) > minimumVelocity) {
            val currentOffset = appBarLayoutView.topAndBottomOffset
            val totalScrollRange = appBarLayoutView.totalScrollRange
            scroller.fling(0, currentOffset, 0, velocityY.toInt(), 0, 0, -totalScrollRange, 0)
            ViewCompat.postInvalidateOnAnimation(this)
          }
        }
        if (!isScrolling) {
          performClick()
        }
        if (nestedScrollAsChild) {
          stopNestedScroll(ViewCompat.TYPE_TOUCH)
          recycleVelocityTracker()
        }
      }
      MotionEvent.ACTION_CANCEL -> {
        if (nestedScrollAsChild) {
          stopNestedScroll(ViewCompat.TYPE_TOUCH)
          recycleVelocityTracker()
        }
      }
    }
    return handledByParent
  }

  fun setConsumeGesture(consume: Boolean) {
    consumeGesture = consume
    if (consume) {
      downEventHandled = false
    }
  }

  fun setInterceptGesture(intercept: Boolean) {
    interceptGesture = intercept
  }

  override fun performClick(): Boolean {
    return super.performClick()
  }

  fun bringAppBarToFront() {
    bringChildToFront(appBarLayoutView)
  }

  override fun onInterceptTouchEvent(event: MotionEvent?): Boolean {
    if (isNativeGestureExcluded()) {
      return false
    }

    if (shouldSkipGestureConsumption(event)) {
      return false
    }

    if (shouldInterceptGesture()) {
      return interceptGesture == true
    }

    interceptTouchEvent = false
    when (event?.action) {
      MotionEvent.ACTION_DOWN -> {
        lastXIntercept = event.x
        lastYIntercept = event.y
        if (!scroller.isFinished) {
          scroller.forceFinished(true)
        }
        if (nestedScrollAsChild) {
          lastTouchY = (event.y + 0.5f).toInt()
          startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, ViewCompat.TYPE_TOUCH)
        }
      }
      MotionEvent.ACTION_UP -> {
        lastXIntercept = event.x
        lastYIntercept = event.y
      }
      MotionEvent.ACTION_MOVE -> {
        if (!scrollEnabledFlag) {
          val x = event.x
          val y = event.y
          if (abs(x - lastXIntercept) >= abs(y - lastYIntercept)) {
            lastXIntercept = x
            lastYIntercept = y
            interceptTouchEvent = false
          } else {
            interceptTouchEvent = true
          }
        }
      }
    }
    return if (!scrollEnabledFlag) interceptTouchEvent else super.onInterceptTouchEvent(event)
  }

  override fun dispatchTouchEvent(event: MotionEvent?): Boolean {
    if (coordinatorHost.isEnableNewGesture) {
      if (consumeGesture != null && consumeGesture == false) {
        return true
      }
      if (consumeGesture != null && !downEventHandled) {
        event?.action = MotionEvent.ACTION_DOWN
        downEventHandled = true
      }
    }
    return super.dispatchTouchEvent(event)
  }

  internal fun removeToolbarView(view: View) {
    toolbarView.visibility = View.GONE
    toolbarView.removeView(view)
  }

  internal fun removeHeaderView(view: View) {
    collapsingToolbarView.removeView(view)
  }

  internal fun removeSlotView(view: View) {
    removeView(view)
  }

  override fun getLayoutResId(): Int {
    return R.layout.scroll_coordinator_layout
  }

  override fun setScrollEnabled(enabled: Boolean) {
    scrollEnabledFlag = enabled
  }

  fun setNestedScrollAsChild(enabled: Boolean) {
    nestedScrollAsChild = enabled
    isNestedScrollingEnabled = enabled
  }

  override fun onStartNestedScroll(child: View, target: View, axes: Int, type: Int): Boolean {
    val superResult = super.onStartNestedScroll(child, target, axes, type)
    if (!nestedScrollAsChild) {
      return superResult
    }
    val vertical = (axes and ViewCompat.SCROLL_AXIS_VERTICAL) != 0
    return (vertical && startNestedScroll(axes)) || superResult
  }

  override fun onNestedPreScroll(target: View, dx: Int, dy: Int, consumed: IntArray, type: Int) {
    if (nestedScrollAsChild) {
      val parentConsumed = intArrayOf(0, 0)
      super.onNestedPreScroll(target, dx, dy, parentConsumed, type)
      val localConsumed = intArrayOf(0, 0)
      dispatchNestedPreScroll(dx, dy, localConsumed, null)
      consumed[0] = parentConsumed[0] + localConsumed[0]
      consumed[1] = parentConsumed[1] + localConsumed[1]
    } else {
      super.onNestedPreScroll(target, dx, dy, consumed, type)
    }
  }

  override fun onNestedPreScroll(target: View, dx: Int, dy: Int, consumed: IntArray) {
    if (nestedScrollAsChild) {
      val parentConsumed = intArrayOf(0, 0)
      super.onNestedPreScroll(target, dx, dy, parentConsumed)
      val localConsumed = intArrayOf(0, 0)
      dispatchNestedPreScroll(dx, dy, localConsumed, null)
      consumed[0] = parentConsumed[0] + localConsumed[0]
      consumed[1] = parentConsumed[1] + localConsumed[1]
    } else {
      super.onNestedPreScroll(target, dx, dy, consumed)
    }
  }

  override fun onNestedScroll(
    target: View,
    dxConsumed: Int,
    dyConsumed: Int,
    dxUnconsumed: Int,
    dyUnconsumed: Int,
    type: Int,
  ) {
    super.onNestedScroll(target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, type)
    if (nestedScrollAsChild && appBarLayoutView.topAndBottomOffset == 0) {
      dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, null, type)
    }
  }

  override fun onNestedScroll(
    target: View,
    dxConsumed: Int,
    dyConsumed: Int,
    dxUnconsumed: Int,
    dyUnconsumed: Int,
  ) {
    super.onNestedScroll(target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed)
    if (nestedScrollAsChild && appBarLayoutView.topAndBottomOffset == 0) {
      dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, null)
    }
  }

  override fun startNestedScroll(axes: Int, type: Int): Boolean {
    return childHelper.startNestedScroll(axes, type)
  }

  override fun stopNestedScroll(type: Int) {
    childHelper.stopNestedScroll(type)
  }

  override fun hasNestedScrollingParent(type: Int): Boolean {
    return childHelper.hasNestedScrollingParent(type)
  }

  override fun dispatchNestedScroll(
    dxConsumed: Int,
    dyConsumed: Int,
    dxUnconsumed: Int,
    dyUnconsumed: Int,
    offsetInWindow: IntArray?,
    type: Int,
  ): Boolean {
    return childHelper.dispatchNestedScroll(
      dxConsumed,
      dyConsumed,
      dxUnconsumed,
      dyUnconsumed,
      offsetInWindow,
      type,
    )
  }

  override fun dispatchNestedPreScroll(
    dx: Int,
    dy: Int,
    consumed: IntArray?,
    offsetInWindow: IntArray?,
    type: Int,
  ): Boolean {
    return childHelper.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type)
  }

  override fun setNestedScrollingEnabled(enabled: Boolean) {
    childHelper.isNestedScrollingEnabled = enabled
  }

  override fun isNestedScrollingEnabled(): Boolean {
    return childHelper.isNestedScrollingEnabled
  }

  override fun startNestedScroll(axes: Int): Boolean {
    return if (nestedScrollAsChild) {
      startNestedScroll(axes, ViewCompat.TYPE_TOUCH)
    } else {
      super.startNestedScroll(axes)
    }
  }

  override fun stopNestedScroll() {
    if (nestedScrollAsChild) {
      stopNestedScroll(ViewCompat.TYPE_TOUCH)
    }
  }

  override fun hasNestedScrollingParent(): Boolean {
    return if (nestedScrollAsChild) {
      hasNestedScrollingParent(ViewCompat.TYPE_TOUCH)
    } else {
      super.hasNestedScrollingParent()
    }
  }

  override fun dispatchNestedPreScroll(
    dx: Int,
    dy: Int,
    consumed: IntArray?,
    offsetInWindow: IntArray?,
  ): Boolean {
    return if (nestedScrollAsChild) {
      dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, ViewCompat.TYPE_TOUCH)
    } else {
      super.dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow)
    }
  }

  override fun dispatchNestedScroll(
    dxConsumed: Int,
    dyConsumed: Int,
    dxUnconsumed: Int,
    dyUnconsumed: Int,
    offsetInWindow: IntArray?,
  ): Boolean {
    return if (nestedScrollAsChild) {
      dispatchNestedScroll(
        dxConsumed,
        dyConsumed,
        dxUnconsumed,
        dyUnconsumed,
        offsetInWindow,
        ViewCompat.TYPE_TOUCH,
      )
    } else {
      super.dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, offsetInWindow)
    }
  }

  override fun dispatchNestedFling(velocityX: Float, velocityY: Float, consumed: Boolean): Boolean {
    return if (nestedScrollAsChild) {
      childHelper.dispatchNestedFling(velocityX, velocityY, consumed)
    } else {
      super.dispatchNestedFling(velocityX, velocityY, consumed)
    }
  }

  override fun dispatchNestedPreFling(velocityX: Float, velocityY: Float): Boolean {
    return if (nestedScrollAsChild) {
      childHelper.dispatchNestedPreFling(velocityX, velocityY)
    } else {
      super.dispatchNestedPreFling(velocityX, velocityY)
    }
  }

  override fun computeScroll() {
    if (nestedScrollAsChild && scroller.computeScrollOffset()) {
      val dy = scroller.currY - appBarLayoutView.topAndBottomOffset
      ViewCompat.postOnAnimation(this) {
        scrollSelf(-dy)
      }
      ViewCompat.postInvalidateOnAnimation(this)
    }
    super.computeScroll()
  }
}
