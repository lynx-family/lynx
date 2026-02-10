// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.refresh

import android.annotation.SuppressLint
import android.content.Context
import android.view.View
import com.lynx.tasm.behavior.ui.view.AndroidView
import com.scwang.smart.refresh.layout.api.RefreshHeader
import com.scwang.smart.refresh.layout.api.RefreshKernel
import com.scwang.smart.refresh.layout.api.RefreshLayout
import com.scwang.smart.refresh.layout.constant.RefreshState
import com.scwang.smart.refresh.layout.constant.SpinnerStyle

class LynxRefreshHeaderView(context: Context?) : AndroidView(context), RefreshHeader {
  @SuppressLint("RestrictedApi")
  override fun onStateChanged(refreshLayout: RefreshLayout, oldState: RefreshState, newState: RefreshState) {
  }

  override fun getView(): View = this

  override fun getSpinnerStyle(): SpinnerStyle = SpinnerStyle.Translate

  override fun isSupportHorizontalDrag(): Boolean = false

  override fun autoOpen(duration: Int, dragRate: Float, animationOnly: Boolean): Boolean = false

  @SuppressLint("RestrictedApi")
  override fun onFinish(refreshLayout: RefreshLayout, success: Boolean): Int {
    return 0
  }

  @SuppressLint("RestrictedApi")
  override fun setPrimaryColors(vararg colors: Int) {
  }

  @SuppressLint("RestrictedApi")
  override fun onInitialized(kernel: RefreshKernel, height: Int, maxDragHeight: Int) {
  }

  @SuppressLint("RestrictedApi")
  override fun onMoving(isDragging: Boolean, percent: Float, offset: Int, height: Int, maxDragHeight: Int) {
  }

  @SuppressLint("RestrictedApi")
  override fun onReleased(refreshLayout: RefreshLayout, height: Int, maxDragHeight: Int) {
  }

  @SuppressLint("RestrictedApi")
  override fun onStartAnimator(refreshLayout: RefreshLayout, height: Int, maxDragHeight: Int) {
  }

  @SuppressLint("RestrictedApi")
  override fun onHorizontalDrag(percentX: Float, offsetX: Int, offsetMax: Int) {
  }
}
