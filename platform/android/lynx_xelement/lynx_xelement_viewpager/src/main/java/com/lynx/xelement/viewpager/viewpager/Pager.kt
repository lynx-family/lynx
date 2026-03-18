// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import com.lynx.xelement.viewpager.childitem.LynxViewpagerItem
import com.lynx.xelement.viewpager.viewpager.BaseCustomViewPager
import com.lynx.tasm.behavior.StyleConstants
import java.util.*

// Copyright 2020 The Lynx Authors. All rights reserved.

abstract class Pager<T : BaseCustomViewPager> (var mViewPager: T, context: Context) : LinearLayout(context) {
  companion object {
    const val TAG = "Foldview#BaseViewPagerImpl"
  }

  interface InterceptTouchEventListener {
    fun interceptTouchEvent(intercept: Boolean)
  }

  private val children: MutableList<LynxViewpagerItem> = ArrayList()
  protected val mPendingChildren: MutableList<LynxViewpagerItem> = ArrayList()
  private var selectIndex: Int = 0
  private val mAdapter = Adapter()
  var isRTLMode = false
  var mChanged: Boolean = true

  init {
    orientation = VERTICAL
    mViewPager.layoutParams = LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.MATCH_PARENT
    )
    mViewPager.adapter = mAdapter
    layoutParams = ViewGroup.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.MATCH_PARENT
    )
    this.addView(mViewPager, 0)
  }

  private fun updateRTLStatus(boolean: Boolean) {
    val adapter = mViewPager.getReversingAdapter()
    val current = mViewPager.currentItem
    isRTLMode = boolean
    mViewPager.setRTL(boolean)
    adapter?.setmIsRtl(boolean)
    mViewPager.currentItem = current
  }

  fun addChildItem(child: LynxViewpagerItem) {
    mChanged = true
    mPendingChildren.add(child)
  }

  fun removeChildItem(child: LynxViewpagerItem) {
    mChanged = true
    mPendingChildren.remove(child)
  }

  fun setKeepItemView(isKeep: Boolean) {
    if (isKeep) {
      mViewPager.offscreenPageLimit = Integer.MAX_VALUE
    } else {
      mViewPager.offscreenPageLimit = 1
    }
  }

  fun setSelectedIndex(index: Int) {
    selectIndex = index
  }

  inner class Adapter : androidx.viewpager.widget.PagerAdapter() {
    override fun getItemPosition(`object`: Any): Int {
      val position = children.indexOf(`object`)
      if (position == -1) {
        return POSITION_NONE
      }
      return position
    }

    override fun getCount() = children.size

    override fun instantiateItem(
      container: ViewGroup,
      position: Int
    ): Any {
      val viewPagerItem = children[position]
      val parent = viewPagerItem.view.parent as ViewGroup?
      parent?.removeView(viewPagerItem.view)
      container.addView(viewPagerItem.view)
      viewPagerItem.sendIsAttachedEvent(true, position)
      return viewPagerItem
    }

    // 判断instantiateItem 返回来的Object与view是否是代表的同一个视图
    override fun isViewFromObject(
      view: View,
      obj: Any
    ): Boolean {
      if(obj is LynxViewpagerItem){
        return view === obj.view
      }
      return false
    }

    override fun destroyItem(container: ViewGroup, position: Int, `object`: Any) {
      if (`object` is LynxViewpagerItem) {
        container.removeView(`object`.view)
        `object`.sendIsAttachedEvent(false, position)
      }

    }

    override fun getPageTitle(position: Int): CharSequence? {
      return null
    }
  }

  fun setCurrentSelectIndex(index: Int) {
    mViewPager.currentItem = index
  }

  fun setCurrentSelectIndex(index: Int, smooth: Boolean) {
    mViewPager.setCurrentItem(index, smooth)
  }

  fun setAllowHorizontalGesture(enable: Boolean) {
    mViewPager.mAllowHorizontalGesture = enable
  }

  fun setLynxDirection(layoutDirection: Int) {
    val isRTL =
      layoutDirection == StyleConstants.DIRECTION_RTL ||
        layoutDirection == StyleConstants.DIRECTION_LYNXRTL
    if (isRTL != isRTLMode) {
      updateRTLStatus(isRTL)
    }
  }

  fun isRTL(): Boolean {
    return isRTLMode
  }

  fun patchFinished() {
    if (mChanged) {
      mChanged = false
      children.clear()
      children.addAll(mPendingChildren)
      mAdapter.notifyDataSetChanged()
      updateCurrentItemAfterPatch()
    }
  }

  private fun updateCurrentItemAfterPatch() {
    val count = mViewPager.adapter?.count ?: 0
    if (count == 0) {
      return
    }
    val target = selectIndex.coerceIn(0, count - 1)
    mViewPager.setCurrentItem(target, false)
  }
}
