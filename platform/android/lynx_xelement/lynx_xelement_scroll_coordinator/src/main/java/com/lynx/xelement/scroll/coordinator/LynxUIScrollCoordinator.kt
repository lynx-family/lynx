// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator

import android.content.Context
import android.view.View
import androidx.core.view.ViewCompat
import com.google.android.material.appbar.AppBarLayout
import com.google.android.material.appbar.CollapsingToolbarLayout
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.LynxViewClient
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.LynxUIMethodConstants
import com.lynx.tasm.behavior.StyleConstants
import com.lynx.tasm.behavior.StylesDiffMap
import com.lynx.tasm.behavior.event.EventTarget
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.UIShadowProxy
import com.lynx.tasm.behavior.ui.scroll.UIScrollView
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper
import com.lynx.tasm.featurecount.LynxFeatureCounter
import com.lynx.tasm.gesture.GestureArenaMember
import com.lynx.tasm.gesture.LynxNewGestureDelegate
import com.lynx.tasm.gesture.detector.GestureDetector
import com.lynx.tasm.gesture.handler.BaseGestureHandler
import com.lynx.tasm.gesture.handler.GestureConstants
import com.lynx.tasm.utils.ContextUtils
import com.lynx.tasm.utils.PixelUtils
import com.lynx.tasm.utils.UIThreadUtils
import com.lynx.tasm.utils.UnitUtils
import com.lynx.xelement.scroll.coordinator.childitem.LynxUIScrollCoordinatorHeader
import com.lynx.xelement.scroll.coordinator.childitem.LynxUIScrollCoordinatorSlot
import com.lynx.xelement.scroll.coordinator.childitem.LynxUIScrollCoordinatorToolbar
import kotlin.math.abs

@LynxGeneratorName(packageName = "com.lynx.xelement.scroll.coordinator")
@LynxBehavior(tagName = ["scroll-coordinator"], isCreateAsync = true)
open class LynxUIScrollCoordinator(
  context: LynxContext?,
  params: Any?,
) : BaseScrollCoordinator<ScrollCoordinatorAppBarLayout, ScrollCoordinatorLayout>(context, params) {
  companion object {
    const val TAG = "ScrollCoordinator"
    private const val DISABLE_TOUCH_STOP_FLING_FEATURE = 81
  }

  private var headerCollapsed = false
  private var toolbarInteractionEnabled = true
  private var headerSupportsScrollView = false
  private var overflowVisible = false
  private var headerSlotOverflowHitTestEnabled = true
  private var headerAboveSlot = false
  private var headerUi: LynxUI<*>? = null
  private var toolbarUi: LynxUI<*>? = null
  private var headerTapSlop = 0

  constructor(context: LynxContext?) : this(context, null)

  override fun createView(context: Context?): ScrollCoordinatorLayout? {
    val activity = ContextUtils.getActivity(lynxContext)
    LLog.i(TAG, "createView start: $this $context realActivity is: $activity")
    val ctx = context ?: return null
    coordinatorLayout = ScrollCoordinatorLayout(ctx, this)
    initializeDefaults(ctx)
    LLog.i(TAG, "createView end: $this, $coordinatorLayout")
    return coordinatorLayout
  }

  override fun updateAuxiliaryViewsOnTree() {
  }

  override fun setOverflow(overflowInt: Int?) {
    super.setOverflow(overflowInt)
    handleOverflow(overflowInt != StyleConstants.OVERFLOW_HIDDEN)
  }

  override fun getHeaderTapSlopSize(): Int {
    return headerTapSlop
  }

  fun handleOverflow(enabled: Boolean) {
    overflowVisible = enabled
    mView.clipChildren = !enabled
    coordinatorLayout.updateAppBarOverflow(enabled)
    tryPromoteHeader()
  }

  fun handleHeaderOverflow(enabled: Boolean) {
    coordinatorLayout.updateHeaderOverflow(enabled)
  }

  fun handleToolbarOverflow(enabled: Boolean) {
    coordinatorLayout.updateToolbarOverflow(enabled)
  }

  fun tryPromoteHeader() {
    if (!isCoordinatorLayoutInitialized()) {
      return
    }
    if (overflowVisible && headerAboveSlot) {
      coordinatorLayout.bringAppBarToFront()
    }
  }

  override fun insertChild(child: LynxBaseUI, index: Int) {
    if (child is LynxUI<*>) {
      mChildren.add(index, child)
      child.parent = this
      when (child) {
        is LynxUIScrollCoordinatorToolbar -> {
          toolbarUi = child
          coordinatorLayout.addToolbarView(child.view)
          coordinatorLayout.updateToolbarOverflow(child.isOverflowEnabled())
        }
        is LynxUIScrollCoordinatorHeader -> {
          headerUi = child
          coordinatorLayout.addHeaderView(child.view)
          coordinatorLayout.updateHeaderOverflow(child.isOverflowEnabled())
        }
        is LynxUIScrollCoordinatorSlot -> {
          coordinatorLayout.addSlotView(child.view)
          tryPromoteHeader()
        }
      }
    }
  }

  override fun removeChild(child: LynxBaseUI) {
    if (child is LynxUI<*>) {
      mChildren.remove(child)
      child.parent = null
      when (child) {
        is LynxUIScrollCoordinatorToolbar -> coordinatorLayout.removeToolbarView(child.view)
        is LynxUIScrollCoordinatorHeader -> coordinatorLayout.removeHeaderView(child.view)
        is LynxUIScrollCoordinatorSlot -> coordinatorLayout.removeSlotView(child.view)
      }
    }
  }

  @LynxProp(name = "tab-movable-enable", defaultBoolean = true)
  fun setTabMovableEnable(enable: Boolean) {
    coordinatorLayout.getAppBarLayout().setToolbarDragEnabled(enable)
  }

  override fun isOffsetSupportHeight(): Boolean = true

  override fun afterPropsUpdated(props: StylesDiffMap?) {
    super.afterPropsUpdated(props)
    if (isEnableScrollMonitor) {
      coordinatorLayout.getAppBarLayout().setScrollListener(
        object : ScrollCoordinatorAppBarLayout.AppBarLayoutScrollListener {
          override fun onScrollStart() {
            lynxContext.lynxViewClient.onScrollStart(
              LynxViewClient.ScrollInfo(view, tagName, scrollMonitorTag),
            )
          }

          override fun onScrollStop() {
            lynxContext.lynxViewClient.onScrollStop(
              LynxViewClient.ScrollInfo(view, tagName, scrollMonitorTag),
            )
          }

          override fun onFling() {
            lynxContext.lynxViewClient.onFling(
              LynxViewClient.ScrollInfo(view, tagName, scrollMonitorTag),
            )
          }
        },
      )
    }
  }

  override fun findUIWithCustomLayout(x: Float, y: Float, parent: UIGroup<*>?): EventTarget {
    var target = super.findUIWithCustomLayout(x, y, parent)

    if (!toolbarInteractionEnabled && isToolbarInteractionDisabled(target)) {
      for (ui in mChildren) {
        if (ui is LynxUIScrollCoordinatorHeader && !headerCollapsed) {
          target = findTargetWithoutToolbar(x, y, this)
          continue
        }
      }
    } else if (headerSupportsScrollView) {
      updateScrollFlag(!isEventTargetInScrollView(target))
    }
    return target
  }

  private fun updateScrollFlag(shouldScroll: Boolean) {
    val layoutParams =
      coordinatorLayout.getCollapsingToolbar().layoutParams as AppBarLayout.LayoutParams
    if (shouldScroll) {
      layoutParams.scrollFlags = layoutParams.scrollFlags or 0x1
    } else {
      layoutParams.scrollFlags = layoutParams.scrollFlags shr 1 shl 1
    }
    coordinatorLayout.getCollapsingToolbar().layoutParams = layoutParams
  }

  override fun isTransformedTouchPointInView(
    inPoint: FloatArray,
    parent: View?,
    child: View?,
    outLocalPoint: FloatArray,
    relations: MutableMap<View, LynxUI<*>>?,
  ): Boolean {
    if (!headerSlotOverflowHitTestEnabled) {
      return super.isTransformedTouchPointInView(inPoint, parent, child, outLocalPoint, relations)
    }
    val currentParent = parent ?: return false
    val currentChild = child ?: return false
    val point = getTargetPoint(
      inPoint[0],
      inPoint[1],
      currentParent.scrollX,
      currentParent.scrollY,
      currentChild,
      currentChild.matrix,
    )
    outLocalPoint[0] = point[0]
    outLocalPoint[1] = point[1]

    var childUi = relations?.get(currentChild)
    if (childUi == null) {
      if (currentChild.javaClass == ScrollCoordinatorAppBarLayout::class.java
        || currentChild.javaClass == CollapsingToolbarLayout::class.java
      ) {
        childUi = headerUi
        if (childUi != null && !childUi.containsPoint(outLocalPoint[0], outLocalPoint[1])) {
          childUi = toolbarUi
        }
      }
    }

    return if (childUi != null) {
      childUi.containsPoint(outLocalPoint[0], outLocalPoint[1])
    } else {
      outLocalPoint[0] >= 0 && outLocalPoint[0] < currentChild.right - currentChild.left &&
        outLocalPoint[1] >= 0 && outLocalPoint[1] < currentChild.bottom - currentChild.top
    }
  }

  private fun findTargetWithoutToolbar(x: Float, y: Float, parent: UIGroup<*>): EventTarget? {
    val children: MutableMap<View, LynxUI<*>> = HashMap()
    for (i in parent.childCount - 1 downTo 0) {
      var child = parent.getChildAt(i)
      if (child is LynxUIScrollCoordinatorToolbar) {
        continue
      }
      if (child is UIShadowProxy) {
        child = child.child
      }
      if (child is LynxUI<*>) {
        children[child.view] = child
      }
    }
    return findUIWithCustomLayoutByChildren(x, y, parent, children)
  }

  private fun isEventTargetInScrollView(target: EventTarget?): Boolean {
    if (target is UIScrollView) {
      return true
    }
    if (target == null || target is LynxUIScrollCoordinator) {
      return false
    }
    return isEventTargetInScrollView(target.parent())
  }

  private fun isToolbarInteractionDisabled(target: EventTarget?): Boolean {
    if (target is LynxUIScrollCoordinatorToolbar && !target.isUserInteractionEnabled) {
      return true
    }
    if (target == null || target is LynxUIScrollCoordinator) {
      return false
    }
    return isToolbarInteractionDisabled(target.parent())
  }

  override fun destroy() {
    super.destroy()
    coordinatorLayout.getAppBarLayout().setScrollListener(null)
  }

  override fun onAppBarOffsetChanged(layout: AppBarLayout?, offset: Int) {
    val appBarLayout = layout ?: return
    headerCollapsed = abs(offset) >= appBarLayout.totalScrollRange
  }

  @LynxProp(name = "toolbar-interaction-enable", defaultBoolean = true)
  fun setToolbarInteractionEnable(enable: Boolean) {
    toolbarInteractionEnabled = enable
  }

  @LynxProp(name = "header-scrollview-enable", defaultBoolean = false)
  fun setHeaderScrollViewEnable(enable: Boolean) {
    headerSupportsScrollView = enable
  }

  /**
   * @name: experimental-header-slot-overflow-hit-test
   * @description: Controls whether the header and slot overflow hit-testing is enabled. When true, uses the current logic that orders hit-testing between header and slot based on their stacking; when false, defers to the default implementation.
   * @category: experimental
   * @standardAction: offline
   * @supportVersion: 3.6
   **/
  @LynxProp(name = "experimental-header-slot-overflow-hit-test", defaultBoolean = true)
  fun setHeaderSlotOverflowHitTest(enable: Boolean) {
    headerSlotOverflowHitTestEnabled = enable
  }

  @LynxProp(name = "android-enable-touch-stop-fling", defaultBoolean = true)
  fun setEnableTouchStopFling(enable: Boolean) {
    coordinatorLayout.findViewById<ScrollCoordinatorAppBarLayout>(R.id.app_bar_layout)
      .setTouchStopFlingEnabled(enable)
    if (!enable) {
      LynxFeatureCounter.count(DISABLE_TOUCH_STOP_FLING_FEATURE, mContext.instanceId)
    }
  }

  /**
   * @name: android-header-over-slot
   * @description: In Android, this property controls whether the header is rendered above the slot when header overflow is enabled.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.10
   **/
  @LynxProp(name = "android-header-over-slot")
  fun setAndroidHeaderOverSlot(enable: Boolean) {
    headerAboveSlot = enable
    tryPromoteHeader()
  }

  /**
   * @name: header-over-slot
   * @description: Controls whether the header is rendered above the slot when overflow is enabled.
   * @category: experimental
   * @standardAction: keep
   * @supportVersion: 2.11
   **/
  @LynxProp(name = "header-over-slot")
  fun setHeaderOverSlot(enable: Boolean) {
    headerAboveSlot = enable
    tryPromoteHeader()
  }

  /**
   * @name: android-nested-scroll-as-child
   * @description: Android uses CoordinatorLayout here. It is a nested scrolling parent by default, so this property enables it to participate as a nested scrolling child when embedded inside other scrolling widgets.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.8
   **/
  @LynxProp(name = "android-nested-scroll-as-child", defaultBoolean = false)
  fun setNestedScrollAsChild(enable: Boolean) {
    coordinatorLayout.setNestedScrollAsChild(enable)
  }

  override fun consumeGesture(consumeGesture: Boolean) {
    mView.setConsumeGesture(consumeGesture)
  }

  override fun interceptGesture(interceptGesture: Boolean) {
    mView.setInterceptGesture(interceptGesture)
  }

  /**
   * @name: android-header-tap-slop
   * @description: The header tap slop on Android is 0 by default. Setting this attribute makes small vertical motion count as both tap and scroll when needed.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.14
   **/
  @LynxProp(name = "android-header-tap-slop", defaultInt = 0)
  fun setAndroidHeaderTapSlop(tapSlop: Int) {
    headerTapSlop = tapSlop
  }

  @LynxUIMethod
  fun scrollBy(params: ReadableMap?, callback: Callback?) {
    if (callback == null) {
      return
    }
    if (params == null || !params.hasKey("offset")) {
      callback.invoke(
        LynxUIMethodConstants.PARAM_INVALID,
        "Invoke scrollBy failed due to param is null",
      )
      return
    }
    val offset = PixelUtils.dipToPx(params.getDouble("offset", 0.0)).toDouble()

    UIThreadUtils.runOnUiThreadImmediately {
      val result = scrollBy(offset.toFloat(), offset.toFloat())
      val response = JavaOnlyMap()
      response.putDouble("consumedX", PixelUtils.pxToDip(result[0]).toInt().toDouble())
      response.putDouble("consumedY", PixelUtils.pxToDip(result[1]).toInt().toDouble())
      response.putDouble("unconsumedX", PixelUtils.pxToDip(result[2]).toInt().toDouble())
      response.putDouble("unconsumedY", PixelUtils.pxToDip(result[3]).toInt().toDouble())
      callback.invoke(LynxUIMethodConstants.SUCCESS, response)
    }
  }

  override fun scrollBy(deltaX: Float, deltaY: Float): FloatArray {
    val result = FloatArray(4)
    setOffsetWithoutAnimation(offsetPx = deltaY.toInt())
    if (abs(deltaX) > Float.MIN_VALUE || abs(deltaY) > Float.MIN_VALUE) {
      recognizeGesturere()
    }
    result[0] = 0f
    result[1] = abs(mView.getAppBarLayout().topAndBottomOffset.toFloat())
    result[2] = deltaX
    result[3] = deltaY - result[1]
    return result
  }

  @LynxUIMethod
  fun setFoldExpanded(params: ReadableMap, callback: Callback? = null) {
    val response = JavaOnlyMap()
    response["success"] = false
    if (params.hasKey("offset")) {
      val offsetValue = params.getString("offset", "")
      var offset = -1
      if (offsetValue.endsWith("px") || offsetValue.endsWith("rpx")) {
        offset = UnitUtils.toPxWithDisplayMetrics(
          offsetValue,
          0.0f,
          -1.0f,
          mContext.screenMetrics,
        ).toInt()
      }
      var enableAnimation = true
      if (params.hasKey("smooth")) {
        enableAnimation = params.getBoolean("smooth")
      }
      if (enableAnimation) {
        animateToOffset(offsetPx = offset)
      } else {
        setOffsetWithoutAnimation(offsetPx = offset)
      }
      response["success"] = true
    } else {
      response["msg"] = "no index key"
    }
    callback?.invoke(LynxUIMethodConstants.SUCCESS, response)
  }

  @LynxUIMethod
  fun getScrollInfo(callback: Callback) {
    val result = JavaOnlyMap()
    result.putInt("scrollX", 0)
    result.putInt(
      "scrollY",
      LynxUIHelper.px2dip(
        mContext,
        abs(mView.getAppBarLayout().topAndBottomOffset.toFloat()),
      ),
    )
    val collapsingHeight = coordinatorLayout.getCollapsingToolbar().height
    val toolbarHeight = coordinatorLayout.getPinnedToolbar().height
    result.putInt(
      "scrollRange",
      LynxUIHelper.px2dip(mContext, (collapsingHeight - toolbarHeight).toFloat()),
    )

    callback.invoke(LynxUIMethodConstants.SUCCESS, result)
  }

  override fun onGestureScrollBy(deltaX: Float, deltaY: Float) {
    if (!isEnableNewGesture) {
      return
    }

    UIThreadUtils.runOnUiThreadImmediately {
      if (mView == null) {
        return@runOnUiThreadImmediately
      }
      mView.scrollBy(0, deltaY.toInt())
    }
  }

  override fun canConsumeGesture(deltaX: Float, deltaY: Float): Boolean {
    if (!isEnableNewGesture || mView == null) {
      return false
    }
    return !(isAtBorder(true) && deltaY < 0 || isAtBorder(false) && deltaY > 0)
  }

  override fun getMemberScrollX(): Int {
    return 0
  }

  override fun getScrollContainerDirection(): Int {
    return GestureConstants.DIRECTION_VERTICAL
  }

  override fun isAtBorder(isStart: Boolean): Boolean {
    return mView.getAppBarLayout().topAndBottomOffset == 0 ||
      mView.getAppBarLayout().topAndBottomOffset == mView.getAppBarLayout().totalScrollRange
  }

  override fun getMemberScrollY(): Int {
    return mView.getAppBarLayout().topAndBottomOffset
  }

  override fun onInvalidate() {
    if (mView != null && isEnableNewGesture) {
      ViewCompat.postInvalidateOnAnimation(mView)
    }
  }

  override fun getGestureHandlers(): MutableMap<Int, BaseGestureHandler>? {
    return super.getGestureHandlers()
  }

  override fun setGestureDetectors(gestureDetectors: MutableMap<Int, GestureDetector>?) {
    super.setGestureDetectors(gestureDetectors)
  }

  override fun isScrollContainer(): Boolean {
    return true
  }
}
