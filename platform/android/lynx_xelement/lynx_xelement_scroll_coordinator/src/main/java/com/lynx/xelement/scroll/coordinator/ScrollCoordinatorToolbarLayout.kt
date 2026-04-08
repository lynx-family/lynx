// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.Toolbar
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.core.view.ViewCompat
import com.google.android.material.appbar.AppBarLayout
import com.google.android.material.appbar.CollapsingToolbarLayout
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.LynxContext

abstract class ScrollCoordinatorToolbarLayout<T : AppBarLayout>(context: Context) :
  CoordinatorLayout(context) {
  companion object {
    const val TAG = "ScrollCoordinatorLayout"
  }

  protected var scrollEnabledFlag = true
  protected var initialScrollFlags = 0
  private var compatContainerPopup = false
  protected var appBarLayoutView: T
  protected var collapsingToolbarView: CollapsingToolbarLayout
  protected var toolbarView: Toolbar

  abstract fun setScrollEnabled(enabled: Boolean)

  fun setCompatContainerPopupEnabled(enabled: Boolean) {
    compatContainerPopup = enabled
  }

  init {
    if (context is LynxContext) {
      LLog.i(TAG, "context is: ${context.baseContext}")
    }
    val inflate = LayoutInflater.from(context).inflate(getLayoutResId(), this, true)
    appBarLayoutView = findViewById(R.id.app_bar_layout)
    collapsingToolbarView = findViewById(R.id.collapsing_toolbar_layout)
    toolbarView = findViewById(R.id.scroll_coordinator_toolbar)
    inflate.layoutParams =
      ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)

    val params = collapsingToolbarView.layoutParams
    if (params is AppBarLayout.LayoutParams) {
      initialScrollFlags = params.scrollFlags
    }
  }

  abstract fun getLayoutResId(): Int

  abstract fun addSlotView(slotView: View)

  internal fun addHeaderView(headerView: View) {
    addViewToCollapsingToolbar(headerView)
  }

  internal fun updateAppBarOverflow(enabled: Boolean) {
    appBarLayoutView.clipChildren = !enabled
  }

  internal fun updateHeaderOverflow(enabled: Boolean) {
    collapsingToolbarView.clipChildren = !enabled
  }

  internal fun updateToolbarOverflow(enabled: Boolean) {
    toolbarView.clipChildren = !enabled
  }

  internal fun addToolbarView(view: View) {
    toolbarView.visibility = View.VISIBLE
    ViewCompat.setPaddingRelative(toolbarView, 0, 0, 0, 0)
    toolbarView.addView(view)
  }

  protected fun addViewToAppBarLayout(view: View) {
    appBarLayoutView.addView(view)
  }

  private fun addViewToCollapsingToolbar(view: View) {
    collapsingToolbarView.addView(view, 0)
  }

  fun getAppBarLayout(): T {
    return appBarLayoutView
  }

  fun getPinnedToolbar(): Toolbar {
    return toolbarView
  }

  fun getCollapsingToolbar(): CollapsingToolbarLayout {
    return collapsingToolbarView
  }

  override fun canScrollVertically(direction: Int): Boolean {
    val behavior = (appBarLayoutView.layoutParams as LayoutParams).behavior
    if (!compatContainerPopup || behavior !is AppBarLayout.Behavior) {
      return super.canScrollVertically(direction)
    }
    return if (behavior.topAndBottomOffset < 0
      || (direction > 0 && behavior.topAndBottomOffset == 0)
    ) {
      true
    } else if (direction < 0 && behavior.topAndBottomOffset == 0) {
      false
    } else {
      super.canScrollVertically(direction)
    }
  }
}
