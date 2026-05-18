// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.viewpager

import android.content.Context
import androidx.viewpager.widget.PagerAdapter
import androidx.viewpager.widget.ViewPager
import android.view.MotionEvent
import android.view.View
import com.lynx.xelement.viewpager.Pager
import com.lynx.xelement.viewpager.ReversingAdapter
import com.lynx.tasm.base.LLog
import java.lang.reflect.Field
import java.util.HashMap

// Copyright 2020 The Lynx Authors. All rights reserved.

open class BaseCustomViewPager(context: Context) : androidx.viewpager.widget.ViewPager(context) {
  var mAllowHorizontalGesture: Boolean = true
  var mInterceptTouchEventListener: Pager.InterceptTouchEventListener? = null
  var mLastMotionX: Float  = 0F
  var mLastMotionY: Float = 0F
  var mActivePointerId: Int = -1
  val superclass: Class<*>? = this.javaClass.superclass?.superclass
  var isRTLMode = false


  private val mPageChangeListeners: HashMap<OnPageChangeListener, ReversingOnPageChangeListener> = HashMap()

  protected fun getIsUnableToDrag(): Field? {
    return try {
      superclass?.getDeclaredField("mIsUnableToDrag")
    } catch (e: NoSuchFieldException) {
      LLog.e(Pager.TAG, "no such field mIsUnableToDrag")
      null
    }
  }

  override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
    return try {
      super.dispatchTouchEvent(ev)
    } catch (e : Exception) {
      LLog.e(Pager.TAG, e.toString())
      false
    }
  }

  override fun canScroll(childView: View, checkSelf: Boolean, dx: Int, x: Int, y: Int): Boolean {
    return if (childView is BaseCustomViewPager) {
      super.canScroll(childView, checkSelf, dx, x, y) && childView.mAllowHorizontalGesture
    } else {
      super.canScroll(childView, checkSelf, dx, x, y)
    }
  }

  override fun onAttachedToWindow() {
    super.onAttachedToWindow()
    try {
      val populate = superclass?.getDeclaredMethod("populate")
      populate?.isAccessible = true
      populate?.invoke(this)
    } catch (e: Throwable) {
      LLog.e(Pager.TAG, "populate failed")
      // Fallback to a new layout pass so ViewPager can populate after attach.
      requestLayout()
    }
  }

  override fun canScrollHorizontally(direction: Int): Boolean {
    return if (!mAllowHorizontalGesture) false else super.canScrollHorizontally(direction)
  }

  override fun addOnPageChangeListener(listener: androidx.viewpager.widget.ViewPager.OnPageChangeListener) {
    val reversingListener = ReversingOnPageChangeListener(listener, this, super.getAdapter())
    mPageChangeListeners[listener] = reversingListener
    super.addOnPageChangeListener(reversingListener)
  }

  override fun removeOnPageChangeListener(listener: androidx.viewpager.widget.ViewPager.OnPageChangeListener) {
    val reverseListener: ReversingOnPageChangeListener? = mPageChangeListeners.remove(listener)
    if (reverseListener != null) {
      super.removeOnPageChangeListener(reverseListener)
    }
  }



  override fun getCurrentItem(): Int {
    var item = super.getCurrentItem()
    val adapter = super.getAdapter()
    if (adapter != null && isRTL()) {
      item = adapter.count - item - 1
    }
    return item
  }

  override fun setAdapter(adapter: androidx.viewpager.widget.PagerAdapter?) {
    var newAdapter = adapter
    if (newAdapter != null && adapter != null) {
      newAdapter = ReversingAdapter(adapter)
    }
    super.setAdapter(newAdapter)
  }

  override fun getAdapter(): androidx.viewpager.widget.PagerAdapter? {
    val adapter = super.getAdapter() as ReversingAdapter?
    return adapter?.getmDelegete()
  }

  fun getReversingAdapter(): ReversingAdapter? {
    return super.getAdapter() as? ReversingAdapter
  }

  fun isRTL(): Boolean {
    return isRTLMode
  }

  fun setRTL(rtlMode: Boolean) {
    isRTLMode = rtlMode
  }
}


internal class ReversingOnPageChangeListener constructor(private val mListener: androidx.viewpager.widget.ViewPager.OnPageChangeListener,
                                                         private val mViewPager: BaseCustomViewPager,
                                                         private val mSuperAdapter: androidx.viewpager.widget.PagerAdapter?) : androidx.viewpager.widget.ViewPager.OnPageChangeListener {

  override fun onPageScrolled(position: Int, positionOffset: Float, positionOffsetPixels: Int) {
    var position = position
    var positionOffset = positionOffset
    var positionOffsetPixels = positionOffsetPixels
    val width = mViewPager.width
    val adapter = mSuperAdapter
    if (mViewPager.isRTL() && adapter != null) {
      val count = adapter.count
      var remainingWidth = (width * (1 - adapter.getPageWidth(position))).toInt() + positionOffsetPixels
      while (position < count && remainingWidth > 0) {
        position += 1
        remainingWidth -= (width * adapter.getPageWidth(position)).toInt()
      }
      position = count - position - 1
      positionOffsetPixels = -remainingWidth
      positionOffset = positionOffsetPixels / (width * adapter.getPageWidth(position))
    }
    try {
      mListener.onPageScrolled(position, positionOffset, positionOffsetPixels)
    } catch (e: NullPointerException) {
      LLog.e("ReversingOnPageChangeListener", e.toString());
    }
  }

  override fun onPageSelected(position: Int) {
    var position = position
    val adapter = mSuperAdapter
    if (mViewPager.isRTL() && adapter != null) {
      position = adapter.count - position - 1
    }
    mListener.onPageSelected(position)
  }

  override fun onPageScrollStateChanged(state: Int) {
    mListener.onPageScrollStateChanged(state)
  }
}
