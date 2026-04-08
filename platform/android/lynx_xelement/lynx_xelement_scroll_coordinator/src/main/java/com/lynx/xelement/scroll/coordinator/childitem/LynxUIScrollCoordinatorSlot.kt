// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator.childitem

import android.content.Context
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.view.AndroidView

@LynxGeneratorName(packageName = "com.lynx.xelement.scroll.coordinator.childitem")
@LynxBehavior(tagName = ["scroll-coordinator-slot"], isCreateAsync = true)
open class LynxUIScrollCoordinatorSlot(
  context: LynxContext,
  params: Any?,
) : UIGroup<AndroidView>(context, params) {
  private var tabOwner: Any? = null
  private var slotDrag: LynxUIScrollCoordinatorSlotDrag? = null
  private var pagerView: Any? = null

  constructor(context: LynxContext) : this(context, null)

  override fun createView(context: Context?): AndroidView = AndroidView(context)

  override fun insertChild(child: LynxBaseUI?, index: Int) {
    super.insertChild(child, index)
    if (child is LynxUI<*>) {
      child.parent = this
      when {
        child is LynxUIScrollCoordinatorSlotDrag -> addSlotDragView(child)
        SlotInterop.hasPagerApi(child.view) -> addPagerView(child.view)
        SlotInterop.hasTabLayoutGetter(child) -> addTabOwner(child)
      }
    }
  }

  private fun addSlotDragView(view: LynxUIScrollCoordinatorSlotDrag) {
    slotDrag = view
    tabOwner?.let { view.setTabOwner(it) }
    pagerView?.let { view.setViewPager(it) }
  }

  private fun addTabOwner(owner: Any) {
    tabOwner = owner
    slotDrag?.setTabOwner(owner)
    pagerView?.let { SlotInterop.attachTabOwner(it, owner) }
  }

  private fun addPagerView(view: Any?) {
    val pager = view ?: return
    pagerView = pager
    slotDrag?.let {
      it.setViewPager(pager)
      tabOwner = it.getTabBarView() ?: tabOwner
    }
    tabOwner?.let { SlotInterop.attachTabOwner(pager, it) }
  }
}

internal object SlotInterop {
  fun hasPagerApi(target: Any?): Boolean {
    val methods = target?.javaClass?.methods ?: return false
    return methods.any { it.name == "setTabLayout" && it.parameterCount == 1 } &&
      methods.any { it.name == "setTabBarElementAdded" && it.parameterCount == 1 }
  }

  fun hasTabLayoutGetter(target: Any?): Boolean {
    return target?.javaClass?.methods?.any { it.name == "getTabLayout" && it.parameterCount == 0 } == true
  }

  fun attachTabOwner(pager: Any, tabOwner: Any) {
    pager.javaClass.methods.firstOrNull {
      it.name == "setTabLayout" &&
        it.parameterCount == 1 &&
        it.parameterTypes[0].isAssignableFrom(tabOwner.javaClass)
    }?.invoke(pager, tabOwner)
    pager.javaClass.methods.firstOrNull {
      it.name == "setTabBarElementAdded" && it.parameterCount == 1
    }?.invoke(pager, false)
  }
}
