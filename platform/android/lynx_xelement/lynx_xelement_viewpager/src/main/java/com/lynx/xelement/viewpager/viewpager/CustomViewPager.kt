// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.viewpager

import android.content.Context
import android.view.MotionEvent


// Copyright 2020 The Lynx Authors. All rights reserved.

open class CustomViewPager(context: Context) : BaseCustomViewPager(context) {
  var mPagerChangeAnimation = true
  var mTouchHandler: TouchEventHandler? = null
  var mForceCanScroll: Boolean = false


  init {
    overScrollMode = OVER_SCROLL_NEVER
  }

  interface TouchEventHandler {
    fun onTouch(ev: MotionEvent?)
  }

  override fun onInterceptTouchEvent(ev: MotionEvent?): Boolean {
    var flag = false
    if (mAllowHorizontalGesture) {
      flag = super.onInterceptTouchEvent(ev)
    }
    return flag
  }

  override fun canScrollHorizontally(direction: Int): Boolean {
    if (mForceCanScroll) {
      return true
    }
    return super.canScrollHorizontally(direction)
  }

  override fun onTouchEvent(ev: MotionEvent?): Boolean {
    when (ev?.actionMasked) {
      MotionEvent.ACTION_UP -> performClick()
    }
    var flag = false
    if (mAllowHorizontalGesture) {
      flag = super.onTouchEvent(ev)
      mTouchHandler?.onTouch(ev)
    } else {
      mInterceptTouchEventListener?.interceptTouchEvent(flag)
    }
    return flag
  }

  override fun performClick(): Boolean {
    return super.performClick()
  }

  override fun setCurrentItem(item: Int) {
    val adapter = super.getAdapter()
    var position = item;
    if (adapter != null && isRTL()) {
      position = adapter.count - position - 1
    }
    super.setCurrentItem(position, mPagerChangeAnimation)
  }


  override fun setCurrentItem(item: Int, smoothScroll: Boolean) {
    val adapter = super.getAdapter()
    var position = item;
    if (adapter != null && isRTL()) {
      position = adapter.count - position - 1
    }
    super.setCurrentItem(position, smoothScroll)
  }

}
