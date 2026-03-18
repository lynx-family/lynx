// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.viewpager

import android.content.Context
import androidx.core.view.ViewCompat
import com.lynx.xelement.viewpager.Pager
import com.lynx.xelement.viewpager.childitem.LynxViewpagerItem
import com.lynx.xelement.viewpager.util.Utils.toARGB
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.PatchFinishListener
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.view.UISimpleView
import com.lynx.tasm.event.EventsListener

// Copyright 2020 The Lynx Authors. All rights reserved.

abstract class BaseLynxViewPager<K: BaseCustomViewPager,T : Pager<K>>
      (context: LynxContext?, params: Any?) : UISimpleView<T>(context, params), PatchFinishListener {
  protected var mEnableChangeEvent: Boolean = false
  protected var mEnableOffsetChangeEvent: Boolean = true
  protected var mCurrentOffset: String = ""
  protected var mIsKeepItemView: Boolean = false
  protected open lateinit var mPager: T
  companion object {
    const val TAG = "LynxViewPager"
    const val BIND_ON_CHANGED = "change"
    const val BIND_OFFSET_CHANGE = "offsetchange"
  }

  constructor(context: LynxContext?) : this(context, null)

  // 提供了一些View的公共属性设置的方法 ,注意注解设置属性的调用时机在这之后
  abstract override fun createView(context: Context?): T?

  protected open fun initListener(context: Context?) {
    initGestureListener()
    initPagerListener()
  }

  private fun initGestureListener() {
    mPager.mViewPager.mInterceptTouchEventListener =
      object : Pager.InterceptTouchEventListener {
        override fun interceptTouchEvent(intercept: Boolean) {
          if (intercept) {
            lynxContext?.onGestureRecognized()
          }
        }
      }
  }

  protected open fun handleFormatStr(format: String, vararg args: Any?): String {
    return String.format(format, args)
  }

  private fun initPagerListener() {
    initViewPagerChangeListener()
  }

  abstract fun sendTabChangeEvent(tag: String, index: Int, scene: String)

  abstract fun initViewPagerChangeListener()


  abstract fun addPagerChildItem(child: LynxViewpagerItem, index: Int)

  override fun insertChild(child: LynxBaseUI, index: Int) {
    if (child is LynxUI<*>) {
      mChildren.add(index, child)
      child.parent = this
      when (child) {
        is LynxViewpagerItem -> {
          addPagerChildItem(child, index)
        }

        else -> {
          LLog.e(TAG, "viewpager child is illegal, please check behaviors")
        }
      }
    }
  }


  override fun removeChild(child: LynxBaseUI) {
    if (child is LynxUI<*>) {
      mChildren.remove(child)
      child.parent = null
      when (child) {
        is LynxViewpagerItem -> {
          mPager.removeChildItem(child)
        }

        else -> {
          LLog.e(TAG, "viewpager child is illegal, please check behaviors")
        }
      }
    }
  }


  override fun layoutChildren() {
    for (index in mChildren.indices) {
      val child = mChildren[index]
      if (!mIsKeepItemView) {
        if (child is LynxUI<*> && !ViewCompat.isAttachedToWindow(child.view)) {
          continue
        }
      }
      if (!needCustomLayout()) {
        if (child is LynxUI<*>) {
          child.layout()
        }
      } else if (child is UIGroup<*>) {
        child.layoutChildren()
      }
    }
  }

  override fun setEvents(events: MutableMap<String, EventsListener>?) {
    super.setEvents(events)
    if (events != null) {
      mEnableChangeEvent = true
    }
  }

  // 注意！！！
  // 默认返回false孩子节点应用lynx底层提供的位置排版信息。
  // 返回true，由UIGroup 的mView,也就是当前的Pager自己去布局孩子。
  override fun needCustomLayout(): Boolean = true

  override fun onPatchFinish() {
    if (mPager.mChanged) {
      mPager.patchFinished()
      selectIndexIsSet(mPager.mViewPager.currentItem)
    }
  }

  @LynxProp(name = "background")
  fun setBackground(color: String) {
    mPager.setBackgroundColor(color.toARGB())
  }

  @LynxProp(name = "allow-horizontal-gesture")
  fun setAllowHorizontalGesture(enable: Boolean) {
    mPager.setAllowHorizontalGesture(enable)
  }

  @LynxProp(name = "select-index")
  fun setSelect(selectIndex: Int) {
    if (mPager.mViewPager.currentItem == selectIndex) {
      return
    }
    if (selectIndex >= 0 && selectIndex < mPager.mViewPager.adapter?.count ?: 0) {
      mPager.setCurrentSelectIndex(selectIndex)
    }
    mPager.setSelectedIndex(selectIndex)
  }

  @LynxProp(name = "keep-item-view")
  fun setKeepItemView(keepItemView: Boolean) {
    mIsKeepItemView = keepItemView
    mPager.setKeepItemView(keepItemView)
  }

  @LynxUIMethod
  abstract fun selectTab(params: ReadableMap, callback: Callback? = null)

  override fun setLynxDirection(direction: Int) {
    super.setLynxDirection(direction)
    mPager.setLynxDirection(direction)
  }

  open fun selectIndexIsSet(index: Int) {
  }
}
