// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator

import android.content.Context
import android.graphics.Rect
import android.view.MotionEvent
import android.view.VelocityTracker
import android.view.View
import android.view.ViewConfiguration
import android.view.ViewGroup
import android.view.ViewTreeObserver
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
    private const val NO_SLOT_BOUNDARY = -1
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
  private var flingGeneration = 0
  private var lastTouchY = 0
  private val scrollOffset = IntArray(2)
  private val scrollConsumed = IntArray(2)
  private val nestedOffsets = IntArray(2)
  private val parentScrollConsumed = IntArray(2)
  private val ancestorScrollConsumed = IntArray(2)
  private var dispatchOffsetUpdatesMethod: Method? = null
  private var slotView: View? = null
  private val visibleRect = Rect()
  private val slotBounds = Rect()
  private val candidateBounds = Rect()
  private val slotLocation = IntArray(2)
  private val candidateLocation = IntArray(2)
  private var cachedScrollableView: View? = null
  private var cachedNonScrollableRoot: View? = null
  private var clampingSlotBoundary = false
  private var adaptToEmptySlot = false
  private val slotScrollChangedListener =
    ViewTreeObserver.OnScrollChangedListener { cachedNonScrollableRoot = null }
  private val offsetStateRestorer =
    AppBarLayout.OnOffsetChangedListener { _, offset ->
      if (offset != appBarLayoutView.topAndBottomOffset) {
        dispatchOffsetUpdates()
      }
    }

  init {
    appBarLayoutView.addOnOffsetChangedListener(
      AppBarLayout.OnOffsetChangedListener { _, _ ->
        if (!clampingSlotBoundary) {
          clampNonScrollableSlotBoundary()
        }
      },
    )
    collapsingToolbarView.addOnAttachStateChangeListener(
      object : View.OnAttachStateChangeListener {
        override fun onViewAttachedToWindow(view: View) {
          appBarLayoutView.removeOnOffsetChangedListener(offsetStateRestorer)
          appBarLayoutView.addOnOffsetChangedListener(offsetStateRestorer)
        }

        override fun onViewDetachedFromWindow(view: View) {
          appBarLayoutView.removeOnOffsetChangedListener(offsetStateRestorer)
        }
      },
    )
  }

  open fun getCollapsingToolbarLayout(): CollapsingToolbarLayout {
    return getCollapsingToolbar()
  }

  override fun addSlotView(slotView: View) {
    updateSlotView(slotView)
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
    slotView.post {
      if (this.slotView === slotView) {
        invalidateSlotScrollCache()
        clampNonScrollableSlotBoundary()
      }
    }
  }

  private fun scrollSelf(dy: Int): Int {
    val params = appBarLayoutView.layoutParams as? LayoutParams ?: return 0
    val behavior = params.behavior as? AppBarLayout.Behavior ?: return 0
    val slotScrollRange = if (dy > 0) getNonScrollableSlotScrollRange() else NO_SLOT_BOUNDARY
    if (slotScrollRange != NO_SLOT_BOUNDARY) {
      clampToSlotBoundary(slotScrollRange)
    }
    val currentOffset = behavior.topAndBottomOffset
    val totalScrollRange =
      if (slotScrollRange != NO_SLOT_BOUNDARY) slotScrollRange else appBarLayoutView.totalScrollRange
    val desiredOffset = currentOffset - dy
    val clampedOffset = desiredOffset.coerceIn(-totalScrollRange, 0)
    if (clampedOffset == currentOffset) {
      if (dy > 0 && slotScrollRange != NO_SLOT_BOUNDARY && abs(currentOffset) >= slotScrollRange) {
        if (!scroller.isFinished) {
          stopOwnFling()
        }
        return dy
      }
      return 0
    }

    withSlotBoundaryCallbackSuppressed {
      behavior.topAndBottomOffset = clampedOffset
      dispatchOffsetUpdates()
    }
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

  private fun stopOwnFling() {
    flingGeneration++
    if (!scroller.isFinished) {
      scroller.forceFinished(true)
    }
  }

  private fun getNonScrollableSlotScrollRange(scrollTarget: View? = null): Int {
    if (!adaptToEmptySlot) {
      return NO_SLOT_BOUNDARY
    }
    val totalScrollRange = appBarLayoutView.totalScrollRange
    val currentSlotView = slotView
    if (currentSlotView == null || currentSlotView.height <= 0) {
      if (totalScrollRange <= 0 || !getGlobalVisibleRect(visibleRect) || visibleRect.height() <= 0) {
        return NO_SLOT_BOUNDARY
      }
      return minOf(
        totalScrollRange,
        (appBarLayoutView.height - visibleRect.height()).coerceAtLeast(0),
      )
    }
    if (totalScrollRange <= 0 || appBarLayoutView.height <= currentSlotView.height) {
      return NO_SLOT_BOUNDARY
    }
    val targetView = scrollTarget?.takeIf { isViewInSubtree(currentSlotView, it) } ?: currentSlotView
    if (canScrollVerticallyInVisibleSubtree(currentSlotView, targetView)) {
      return NO_SLOT_BOUNDARY
    }
    return minOf(totalScrollRange, appBarLayoutView.height - currentSlotView.height)
  }

  private fun updateSlotView(view: View?) {
    if (slotView === view) {
      return
    }
    slotView?.viewTreeObserver?.takeIf { it.isAlive }
      ?.removeOnScrollChangedListener(slotScrollChangedListener)
    slotView = view
    cachedScrollableView = null
    cachedNonScrollableRoot = null
    view?.viewTreeObserver?.addOnScrollChangedListener(slotScrollChangedListener)
  }

  private fun invalidateSlotScrollCache() {
    cachedNonScrollableRoot = null
  }

  private inline fun <T> withSlotBoundaryCallbackSuppressed(block: () -> T): T {
    val wasClampingSlotBoundary = clampingSlotBoundary
    clampingSlotBoundary = true
    return try {
      block()
    } finally {
      clampingSlotBoundary = wasClampingSlotBoundary
    }
  }

  private fun clampToSlotBoundary(scrollRange: Int): Boolean {
    val params = appBarLayoutView.layoutParams as? LayoutParams ?: return false
    val behavior = params.behavior as? AppBarLayout.Behavior ?: return false
    val minOffset = -scrollRange
    if (behavior.topAndBottomOffset >= minOffset) {
      return false
    }
    withSlotBoundaryCallbackSuppressed {
      behavior.topAndBottomOffset = minOffset
      dispatchOffsetUpdates()
    }
    return true
  }

  private fun isViewInSubtree(root: View, view: View): Boolean {
    var current: View? = view
    while (current != null) {
      if (current === root) {
        return true
      }
      current = current.parent as? View
    }
    return false
  }

  private fun canScrollVerticallyInVisibleSubtree(slot: View, root: View): Boolean {
    if (cachedNonScrollableRoot === root) {
      return false
    }
    updateSlotBounds(slot)
    if (cachedScrollableView === root &&
      isViewWithinSlotBounds(root) &&
      canScrollVertically(root)
    ) {
      return true
    }
    cachedScrollableView?.let { cachedView ->
      if (isViewInSubtree(root, cachedView) &&
        isViewWithinSlotBounds(cachedView) &&
        canScrollVertically(cachedView)
      ) {
        return true
      }
    }
    cachedScrollableView = null
    cachedScrollableView = findScrollableViewInVisibleSubtree(root)
    cachedNonScrollableRoot = if (cachedScrollableView == null) root else null
    return cachedScrollableView != null
  }

  private fun findScrollableViewInVisibleSubtree(view: View): View? {
    if (!isViewWithinSlotBounds(view)) {
      return null
    }
    if (canScrollVertically(view)) {
      return view
    }
    if (view is ViewGroup) {
      for (index in 0 until view.childCount) {
        val scrollableView = findScrollableViewInVisibleSubtree(view.getChildAt(index))
        if (scrollableView != null) {
          return scrollableView
        }
      }
    }
    return null
  }

  private fun canScrollVertically(view: View): Boolean {
    return view.canScrollVertically(1) || view.canScrollVertically(-1)
  }

  private fun updateSlotBounds(slot: View) {
    slot.getLocationOnScreen(slotLocation)
    slotBounds.set(
      slotLocation[0],
      slotLocation[1],
      slotLocation[0] + slot.width,
      slotLocation[1] + slot.height,
    )
  }

  private fun isViewWithinSlotBounds(view: View): Boolean {
    if (!view.isShown || view.width <= 0 || view.height <= 0) {
      return false
    }
    view.getLocationOnScreen(candidateLocation)
    candidateBounds.set(
      candidateLocation[0],
      candidateLocation[1],
      candidateLocation[0] + view.width,
      candidateLocation[1] + view.height,
    )
    return Rect.intersects(slotBounds, candidateBounds)
  }

  private fun consumeUpwardScrollAtSlotBoundary(
    dy: Int,
    consumed: IntArray,
    scrollRange: Int,
  ): Boolean {
    if (dy <= 0 || scrollRange == NO_SLOT_BOUNDARY) {
      return false
    }
    if (abs(appBarLayoutView.topAndBottomOffset) < scrollRange) {
      return false
    }
    clampToSlotBoundary(scrollRange)
    consumed[1] += dy
    if (!scroller.isFinished) {
      stopOwnFling()
    }
    return true
  }

  private fun getNestedPreScrollDy(dy: Int, scrollRange: Int): Int {
    if (dy <= 0 || scrollRange == NO_SLOT_BOUNDARY) {
      return dy
    }
    clampToSlotBoundary(scrollRange)
    val remaining = scrollRange - abs(appBarLayoutView.topAndBottomOffset)
    return dy.coerceAtMost(remaining.coerceAtLeast(0))
  }

  private fun clampNonScrollableSlotBoundary(): Boolean {
    val scrollRange = getNonScrollableSlotScrollRange()
    if (scrollRange == NO_SLOT_BOUNDARY) {
      return false
    }
    return clampToSlotBoundary(scrollRange).also { clamped ->
      if (clamped) {
        stopOwnFling()
        appBarLayoutView.stopFling()
      }
    }
  }

  override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
    super.onLayout(changed, left, top, right, bottom)
    invalidateSlotScrollCache()
    clampNonScrollableSlotBoundary()
  }

  fun stopFling() {
    stopOwnFling()
    appBarLayoutView.stopFling()
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
        stopOwnFling()
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
            flingGeneration++
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
        stopOwnFling()
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
    if (slotView === view) {
      updateSlotView(null)
    }
    removeView(view)
  }

  internal fun releaseSlotTracking() {
    updateSlotView(null)
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

  fun setAdaptToEmptySlot(enabled: Boolean) {
    adaptToEmptySlot = enabled
    if (enabled) {
      clampNonScrollableSlotBoundary()
    }
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
    val slotScrollRange =
      if (dy > 0) getNonScrollableSlotScrollRange(target) else NO_SLOT_BOUNDARY
    val preScrollDy = getNestedPreScrollDy(dy, slotScrollRange)
    if (nestedScrollAsChild) {
      parentScrollConsumed[0] = 0
      parentScrollConsumed[1] = 0
      withSlotBoundaryCallbackSuppressed {
        super.onNestedPreScroll(target, dx, preScrollDy, parentScrollConsumed, type)
      }
      if (consumeUpwardScrollAtSlotBoundary(
          dy - parentScrollConsumed[1],
          parentScrollConsumed,
          slotScrollRange,
        )
      ) {
        consumed[0] = parentScrollConsumed[0]
        consumed[1] = parentScrollConsumed[1]
        return
      }
      ancestorScrollConsumed[0] = 0
      ancestorScrollConsumed[1] = 0
      dispatchNestedPreScroll(dx, dy, ancestorScrollConsumed, null)
      consumed[0] = parentScrollConsumed[0] + ancestorScrollConsumed[0]
      consumed[1] = parentScrollConsumed[1] + ancestorScrollConsumed[1]
    } else {
      withSlotBoundaryCallbackSuppressed {
        super.onNestedPreScroll(target, dx, preScrollDy, consumed, type)
      }
      consumeUpwardScrollAtSlotBoundary(dy - consumed[1], consumed, slotScrollRange)
    }
  }

  override fun onNestedPreScroll(target: View, dx: Int, dy: Int, consumed: IntArray) {
    val slotScrollRange =
      if (dy > 0) getNonScrollableSlotScrollRange(target) else NO_SLOT_BOUNDARY
    val preScrollDy = getNestedPreScrollDy(dy, slotScrollRange)
    if (nestedScrollAsChild) {
      parentScrollConsumed[0] = 0
      parentScrollConsumed[1] = 0
      withSlotBoundaryCallbackSuppressed {
        super.onNestedPreScroll(target, dx, preScrollDy, parentScrollConsumed)
      }
      if (consumeUpwardScrollAtSlotBoundary(
          dy - parentScrollConsumed[1],
          parentScrollConsumed,
          slotScrollRange,
        )
      ) {
        consumed[0] = parentScrollConsumed[0]
        consumed[1] = parentScrollConsumed[1]
        return
      }
      ancestorScrollConsumed[0] = 0
      ancestorScrollConsumed[1] = 0
      dispatchNestedPreScroll(dx, dy, ancestorScrollConsumed, null)
      consumed[0] = parentScrollConsumed[0] + ancestorScrollConsumed[0]
      consumed[1] = parentScrollConsumed[1] + ancestorScrollConsumed[1]
    } else {
      withSlotBoundaryCallbackSuppressed {
        super.onNestedPreScroll(target, dx, preScrollDy, consumed)
      }
      consumeUpwardScrollAtSlotBoundary(dy - consumed[1], consumed, slotScrollRange)
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
      val generation = flingGeneration
      val dy = scroller.currY - appBarLayoutView.topAndBottomOffset
      ViewCompat.postOnAnimation(this) {
        if (generation == flingGeneration) {
          scrollSelf(-dy)
        }
      }
      ViewCompat.postInvalidateOnAnimation(this)
    }
    super.computeScroll()
  }
}
