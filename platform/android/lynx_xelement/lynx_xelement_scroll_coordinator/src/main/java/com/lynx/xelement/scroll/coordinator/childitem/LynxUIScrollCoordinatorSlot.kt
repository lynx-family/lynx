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
    val resolvedOwner = SlotInterop.resolveTabOwner(owner) ?: owner
    tabOwner = resolvedOwner
    slotDrag?.setTabOwner(resolvedOwner)
    pagerView?.let { SlotInterop.attachTabOwner(it, resolvedOwner) }
  }

  private fun addPagerView(view: Any?) {
    val pager = view ?: return
    pagerView = pager
    slotDrag?.let {
      it.setViewPager(pager)
      tabOwner = SlotInterop.resolveTabOwner(it.getTabBarView()) ?: tabOwner
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

  fun resolveTabOwner(target: Any?): Any? {
    if (hasTabLayoutGetter(target)) {
      return target
    }
    val methods = target?.javaClass?.methods ?: return null
    val ownerCandidates =
      methods
        .filter { it.parameterCount == 0 && (it.name == "getTabBarView" || it.name == "getTabOwner") }
        .mapNotNull { method ->
          kotlin.runCatching { method.invoke(target) }.getOrNull()
        }
    return ownerCandidates.firstOrNull { hasTabLayoutGetter(it) }
  }

  fun attachTabOwner(pager: Any, tabOwner: Any) {
    val resolvedOwner = resolveTabOwner(tabOwner) ?: tabOwner
    val setTabLayoutMethod =
      pager.javaClass.methods.firstOrNull {
        it.name == "setTabLayout" &&
          it.parameterCount == 1 &&
          it.parameterTypes[0].isInstance(resolvedOwner)
      } ?: pager.javaClass.methods.firstOrNull {
        it.name == "setTabLayout" &&
          it.parameterCount == 1 &&
          it.parameterTypes[0].isAssignableFrom(resolvedOwner.javaClass)
      }
    setTabLayoutMethod?.invoke(pager, resolvedOwner)
    pager.javaClass.methods.firstOrNull {
      it.name == "setTabBarElementAdded" && it.parameterCount == 1
    }?.invoke(pager, false)
  }
}
