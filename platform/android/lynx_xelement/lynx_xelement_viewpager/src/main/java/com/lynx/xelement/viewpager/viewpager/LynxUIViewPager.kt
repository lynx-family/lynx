// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.viewpager

import android.content.Context
import android.view.MotionEvent
import com.lynx.xelement.viewpager.childitem.LynxViewpagerItem
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.LynxViewClient.ScrollInfo
import com.lynx.tasm.behavior.*
import com.lynx.tasm.event.LynxDetailEvent
import java.util.*

// Copyright 2020 The Lynx Authors. All rights reserved.

open class LynxUIViewPager(context: LynxContext?, params: Any?) : BaseLynxViewPager<CustomViewPager, ViewPagerImpl>(context, params) {
  private var mDragged: Boolean = false
  private var mLastOffset: Float = 0.0f
  private var mDistinguishTapSwap: Boolean = true
  private var mSinglePageTouch: Boolean = false
  private var mIndexSelectedWhileIdle: Int = 0
  private var mInitialSelectIndex = INVALID_VIEW_PAGER_INDEX
  private var mDraggingDetectedByDraggingAndActionUp: Boolean = false


  companion object {
    const val BIND_ON_WILL_CHANGED = "willchange"
    const val INVALID_VIEW_PAGER_INDEX = -1
  }

  constructor(context: LynxContext?) : this(context, null)

  // 提供了一些View的公共属性设置的方法 ,注意注解设置属性的调用时机在这之后
  override fun createView(context: Context?): ViewPagerImpl? {
    if (context == null) return null
    mPager = ViewPagerImpl(context)
    initListener(context)
    return mPager
  }


  override fun onNodeReload() {
    super.onNodeReload()
    mPager.setCurrentSelectIndex(0, false)
  }

  override fun initListener(context: Context?) {
    super.initListener(context)
    mPager.mViewPager.mTouchHandler = object : CustomViewPager.TouchEventHandler {
      override fun onTouch(ev: MotionEvent?) {
        when (ev?.actionMasked) {
          MotionEvent.ACTION_UP -> {
            if (mDraggingDetectedByDraggingAndActionUp) {
              sendTabWillChangeEvent(mPager.mViewPager.currentItem, true)
            }
            mDraggingDetectedByDraggingAndActionUp = false;
          }
          MotionEvent.ACTION_CANCEL -> {
            if (mDraggingDetectedByDraggingAndActionUp) {
              sendTabWillChangeEvent(mPager.mViewPager.currentItem, true)
            }
            mDraggingDetectedByDraggingAndActionUp = false;
          }
        }
      }
    }
  }

  override fun addPagerChildItem(child: LynxViewpagerItem, index: Int) {
      mPager.addChildItem(child, index)
  }


  override fun handleFormatStr(format: String, vararg args: Any?): String {
    return String.format(Locale.ENGLISH, format, args)
  }


  @LynxProp(name = "page-change-animation")
  fun setPageChangeAnimation(enable: Boolean) {
    mPager.setPagerChangeAnimation(enable)
  }

  /**
   * @name: android-force-can-scroll
   * @description: This attribute is used to control if at start/end of viewpager and set true, gesture can not pass to parent.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 3.0
   */
  @LynxProp(name = "android-force-can-scroll")
  fun setForceCanScroll(enable: Boolean) {
    mPager.setForceCanScroll(enable)
  }

  /**
   * @name: single-page-touch
   * @description: This attribute is used to control only slide one page at a time.
   * @category: standardized
   * @standardAction: keep
   * @supportVersion: 2.14
   **/
  @LynxProp(name = "single-page-touch")
  fun setSinglePageTouch(enable: Boolean) {
    mSinglePageTouch = enable
  }

  /**
   * @name: android-always-overscroll
   * @description: In Android's ViewPager, this attribute is used to control the interaction behavior when scrolling to the edges. Setting it to true enables the default bounce effect, while setting it to false disables the bounce effect. The default value is false.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.11
   **/
  @LynxProp(name = "android-always-overscroll", defaultBoolean = false)
  fun setAndroidAlwaysOverscroll(isAlwaysOverScroll: Boolean) {
    mPager.setViewPagerOverScrollMode(isAlwaysOverScroll)
  }
  /**
   * @name: android-distinguish-swipe-tap
   * @description: On Android, sliding left or right can trigger a tap event. This property can avoid this behavior. It is currently a fallback property to avoid breaking due to changes, and it is set to true by default.
   * @category: temporary
   * @standardAction: remove
   * @supportVersion: 2.10
   * @resolveVersion: 2.12
   **/
  @LynxProp(name = "android-distinguish-swipe-tap", defaultBoolean = true)
  fun setDistinguishSwipeTap(isDistinguish : Boolean) {
    mDistinguishTapSwap = isDistinguish
  }

  @LynxProp(name = "initial-select-index")
  fun setInitialSelect(selectIndex: Int) {
    mInitialSelectIndex = selectIndex;
  }

  @LynxProp(name = "enable-scroll")
  fun setEnableScroll(enable: Boolean) {
    mPager.setAllowHorizontalGesture(enable)
  }

  @LynxUIMethod
  override fun selectTab(params: ReadableMap, callback: Callback?) {
    val response = JavaOnlyMap()
    response["success"] = false
    if (params.hasKey("index")) {
      val index = params.getInt("index")
      if (index >= 0 && index < mPager.mViewPager.adapter?.count ?: 0) {
        sendTabWillChangeEvent(index, false)
        if (params.hasKey("smooth")) {
          val smooth = params.getBoolean("smooth", true);
          mPager.setCurrentSelectIndex(index, smooth)
          if (!smooth) {
            // send event directly
            if (mIndexSelectedWhileIdle != mPager.mViewPager.currentItem) {
              mIndexSelectedWhileIdle = mPager.mViewPager.currentItem
              sendTabChangeEvent("", mIndexSelectedWhileIdle, "")
            }
          }
        } else {
          mPager.setCurrentSelectIndex(index, true)
        }
        response["success"] = true
        callback?.invoke(LynxUIMethodConstants.SUCCESS, response)
      } else {
        response["msg"] = "index out of bounds"
        callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, response)
      }
    } else {
      response["msg"] = "no index key"
      callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, response)
    }
  }


  override fun setOverflow(overflow: Int) {
    super.setOverflow(overflow)
    var isOverFlowHidden = false
    if (overflow == StyleConstants.OVERFLOW_HIDDEN) {
      isOverFlowHidden = true
    }
    mPager.clipChildren = isOverFlowHidden
    mPager.mViewPager.clipChildren = isOverFlowHidden
  }

  @LynxUIMethod
  fun setDragGesture(params: ReadableMap, callback: Callback?) {
    val response = JavaOnlyMap()
    response["success"] = false
    if (params.hasKey("canDrag")) {
      val allow = params.getBoolean("canDrag")
      setAllowHorizontalGesture(allow)
    }
    callback?.invoke(LynxUIMethodConstants.SUCCESS, response)
  }

  override fun initViewPagerChangeListener() {
    mPager.mViewPager.addOnPageChangeListener(object : androidx.viewpager.widget.ViewPager.OnPageChangeListener {
      override fun onPageScrolled(position: Int, positionOffset: Float, positionOffsetPixels: Int) {
        recognizeGestureWhenScroll(positionOffset)
        if (mEnableOffsetChangeEvent) {
          val offsetStr = String.format(Locale.ENGLISH, "%.2f", position + positionOffset)
          if (offsetStr == mCurrentOffset) {
            return
          }
          mCurrentOffset = offsetStr
          sendOffsetChangeEvent(offsetStr)
        }
      }

      override fun onPageScrollStateChanged(state: Int) {
        when (state) {
          androidx.viewpager.widget.ViewPager.SCROLL_STATE_IDLE -> {
            if (mSinglePageTouch) {
              mPager.setAllowHorizontalGesture(true)
            }
            if (isEnableScrollMonitor) {
              lynxContext.lynxViewClient.onScrollStop(
                ScrollInfo(view, tagName, scrollMonitorTag)
              )
            }
            if (mEnableChangeEvent) {
              if (mIndexSelectedWhileIdle != mPager.mViewPager.currentItem) {
                mIndexSelectedWhileIdle = mPager.mViewPager.currentItem
                sendTabChangeEvent("", mIndexSelectedWhileIdle, "")
              }
            }
            mDragged = false
          }
          androidx.viewpager.widget.ViewPager.SCROLL_STATE_DRAGGING -> {
            mDragged = true
            mDraggingDetectedByDraggingAndActionUp = true
            if (isEnableScrollMonitor) {
              lynxContext.lynxViewClient.onScrollStart(
                ScrollInfo(view, tagName, scrollMonitorTag)
              )
            }
          }
          androidx.viewpager.widget.ViewPager.SCROLL_STATE_SETTLING -> {
            if (mSinglePageTouch) {
              mPager.setAllowHorizontalGesture(false)
            }
            if (isEnableScrollMonitor) {
              lynxContext.lynxViewClient.onFling(
                ScrollInfo(view, tagName, scrollMonitorTag)
              )
            }
          }
        }
      }
      override fun onPageSelected(index: Int) {
      }
    })
  }

  override fun onNodeReady() {
    super.onNodeReady()
    if (mFirstRender && mInitialSelectIndex != INVALID_VIEW_PAGER_INDEX) {
      if (mInitialSelectIndex >= 0 && mInitialSelectIndex < (mPager.mViewPager.adapter?.count ?: 0)) {
        mPager.setCurrentSelectIndex(mInitialSelectIndex)
        mIndexSelectedWhileIdle = mPager.mViewPager.currentItem
      }
    }
    mFirstRender = false
  }

  private fun recognizeGestureWhenScroll(positionOffset: Float) {
    if (mDistinguishTapSwap) {
      if (mLastOffset != positionOffset) {
        recognizeGesturere()
        mLastOffset = positionOffset
      }
    }
  }

  fun sendOffsetChangeEvent(offset: String) {
    lynxContext.eventEmitter.sendCustomEvent(
      LynxDetailEvent(sign, BIND_OFFSET_CHANGE).apply {
        addDetail("offset", offset)
      }
    )
  }

  override fun sendTabChangeEvent(tag: String, index: Int, scene: String) {
    lynxContext.eventEmitter.sendCustomEvent(
      LynxDetailEvent(sign, BIND_ON_CHANGED).apply {
        addDetail("isDragged", mDragged)
        addDetail("index", index)
      }
    )
  }

  private fun sendTabWillChangeEvent(index: Int, dragged: Boolean) {
    lynxContext.eventEmitter.sendCustomEvent(
      LynxDetailEvent(sign, BIND_ON_WILL_CHANGED).apply {
        addDetail("isDragged", dragged)
        addDetail("index", index)
      }
    )
  }

  override fun selectIndexIsSet(index: Int) {
    if (mIndexSelectedWhileIdle != index) {
      mIndexSelectedWhileIdle = index
      sendTabChangeEvent("", mIndexSelectedWhileIdle, "")
    }
  }

  override fun isScrollContainer(): Boolean {
    return true
  }
}
