// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator.childitem

import android.content.Context
import android.view.MotionEvent
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.StyleConstants
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.view.AndroidView
import com.lynx.xelement.scroll.coordinator.LynxUIScrollCoordinator

@LynxGeneratorName(packageName = "com.lynx.xelement.scroll.coordinator.childitem")
@LynxBehavior(tagName = ["scroll-coordinator-header"], isCreateAsync = true)
open class LynxUIScrollCoordinatorHeader(
  context: LynxContext,
  params: Any?,
) : UIGroup<ScrollCoordinatorHeaderView>(context, params) {
  private var overflowEnabled = false

  constructor(context: LynxContext) : this(context, null)

  override fun createView(context: Context?): ScrollCoordinatorHeaderView {
    return ScrollCoordinatorHeaderView(context)
  }

  override fun setOverflow(overflowInt: Int?) {
    super.setOverflow(overflowInt)
    updateOverflow(overflowInt != StyleConstants.OVERFLOW_HIDDEN)
  }

  private fun updateOverflow(enabled: Boolean) {
    overflowEnabled = enabled
    mView.clipChildren = !enabled
    if (parent is LynxUIScrollCoordinator) {
      (parent as LynxUIScrollCoordinator).handleHeaderOverflow(enabled)
    }
  }

  fun isOverflowEnabled(): Boolean {
    return isOverflow()
  }

  open fun isOverflow(): Boolean {
    return overflowEnabled
  }

  /**
   * @name: android-consume-gesture
   * @description: Whether to consume all gestures in the header area. If true, all gestures in the header are handed to the header's children for processing.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.17
   **/
  @LynxProp(name = "android-consume-gesture", defaultBoolean = false)
  fun setHandleGesture(value: Boolean) {
    mView.handleGesture = value
  }
}

class ScrollCoordinatorHeaderView(context: Context?) : AndroidView(context) {
  var handleGesture = false

  override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
    if (ev.actionMasked == MotionEvent.ACTION_DOWN && handleGesture) {
      parent?.requestDisallowInterceptTouchEvent(true)
    }
    return super.dispatchTouchEvent(ev)
  }
}
