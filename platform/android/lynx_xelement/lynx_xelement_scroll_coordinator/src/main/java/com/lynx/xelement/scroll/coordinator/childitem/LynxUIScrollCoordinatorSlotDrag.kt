// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator.childitem

import android.content.Context
import android.graphics.Rect
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.StyleConstants
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.scroll.AbsLynxUIScroll
import java.lang.ref.WeakReference

@LynxGeneratorName(packageName = "com.lynx.xelement.scroll.coordinator.childitem")
@LynxBehavior(tagName = ["scroll-coordinator-slot-drag"], isCreateAsync = true)
open class LynxUIScrollCoordinatorSlotDrag(
  context: LynxContext,
  params: Any?,
) : AbsLynxUIScroll<ScrollCoordinatorNestedScrollView>(context, params) {
  private var tabOwner: Any? = null
  private var pagerView: WeakReference<Any>? = null

  constructor(context: LynxContext) : this(context, null)

  override fun createView(context: Context): ScrollCoordinatorNestedScrollView {
    return ScrollCoordinatorNestedScrollView(context)
  }

  override fun insertChild(child: LynxBaseUI?, index: Int) {
    super.insertChild(child, index)
    if (child is LynxUI<*>) {
      child.parent = this
      if (SlotInterop.hasTabLayoutGetter(child)) {
        addTabOwner(child)
      }
    }
  }

  open fun attachPagerView(view: Any) {
    pagerView = WeakReference(view)
    tabOwner?.let { SlotInterop.attachTabOwner(view, it) }
  }

  open fun setTabOwner(owner: Any) {
    addTabOwner(owner)
  }

  private fun addTabOwner(owner: Any) {
    tabOwner = owner
    pagerView?.get()?.let { SlotInterop.attachTabOwner(it, owner) }
  }

  open fun getTabOwner(): Any? {
    return tabOwner
  }

  open fun setViewPager(viewPager: Any) {
    attachPagerView(viewPager)
  }

  open fun getTabBarView(): Any? {
    return getTabOwner()
  }

  override fun setOverflow(overflowInt: Int?) {
    super.setOverflow(overflowInt)
    updateOverflow(overflowInt != StyleConstants.OVERFLOW_HIDDEN)
  }

  private fun updateOverflow(enabled: Boolean) {
    mView.clipChildren = !enabled
    mView.linearLayout.clipChildren = !enabled
    mView.hScrollView.clipChildren = !enabled
  }

  override fun measure() {
    val horizontal = mView.orientation == ScrollCoordinatorNestedScrollView.HORIZONTAL
    var measuredWidth = width
    var measuredHeight = height
    for (i in 0 until childCount) {
      val child = getChildAt(i)
      if (horizontal) {
        measuredWidth = maxOf(measuredWidth, child.width + child.left)
      } else {
        measuredHeight = maxOf(measuredHeight, child.height + child.top)
      }
    }
    mView.setMeasuredSize(measuredWidth, measuredHeight)
    super.measure()
  }

  override fun onLayoutUpdated() {
    super.onLayoutUpdated()
    val paddingLeft = mPaddingLeft + mBorderLeftWidth
    val paddingRight = mPaddingRight + mBorderRightWidth
    val paddingTop = mPaddingTop + mBorderTopWidth
    val paddingBottom = mPaddingBottom + mBorderBottomWidth
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom)
  }

  override fun setEnableScroll(value: Boolean) {
    view.setOnTouchListener { _, _ -> !value }
  }

  override fun invalidate() {
    super.invalidate()
    mView.linearLayout?.invalidate()
  }

  @LynxProp(name = "enable-drag", defaultBoolean = true)
  fun setEnableDrag(value: Boolean) {
    setEnableScroll(value)
  }

  override fun setScrollY(enable: Boolean) {
  }

  override fun setScrollX(enable: Boolean) {
  }

  override fun setScrollBarEnable(value: Boolean) {
  }

  override fun setUpperThreshole(value: Int) {
  }

  override fun setLowerThreshole(value: Int) {
  }

  override fun setScrollTop(value: Int) {
  }

  override fun setScrollLeft(value: Int) {
  }

  override fun scrollToIndex(index: Int) {
  }

  override fun setScrollTap(value: Boolean) {
  }

  override fun sendCustomEvent(l: Int, t: Int, oldl: Int, oldt: Int, type: String?) {
  }

  override fun getScrollX(): Int {
    return mView.scrollX
  }

  override fun getScrollY(): Int {
    return mView.scrollY
  }

  override fun getBoundRectForOverflow(): Rect? {
    return null
  }
}
