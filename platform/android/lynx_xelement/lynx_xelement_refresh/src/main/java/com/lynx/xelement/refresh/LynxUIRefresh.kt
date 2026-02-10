// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.refresh

import android.content.Context
import android.graphics.PointF
import android.graphics.Rect
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.animation.AnimationUtils
import android.widget.HorizontalScrollView
import androidx.core.widget.NestedScrollView
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.LynxError
import com.lynx.tasm.LynxSubErrorCode
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.*
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.swiper.ViewPager
import com.lynx.tasm.event.LynxCustomEvent
import com.lynx.tasm.event.LynxDetailEvent
import com.scwang.smart.refresh.layout.SmartRefreshLayout
import com.scwang.smart.refresh.layout.api.RefreshHeader
import com.scwang.smart.refresh.layout.api.RefreshLayout
import com.scwang.smart.refresh.layout.constant.RefreshState
import com.scwang.smart.refresh.layout.listener.OnMultiListener
import com.scwang.smart.refresh.layout.simple.SimpleMultiListener
import com.scwang.smart.refresh.layout.util.SmartUtil
import com.scwang.smart.refresh.layout.wrapper.RefreshContentWrapper

@LynxGeneratorName(packageName = "com.lynx.xelement.refresh")
@LynxBehavior(tagName = ["refresh"], isCreateAsync = false)
open class LynxUIRefresh(context: LynxContext?, params: Any?): UIGroup<SmartRefreshLayout>(context, params) {
  companion object {
    const val TAG = "LynxUIRefresh"
    const val START_REFRESH = "startrefresh"
    const val START_REFRESH_PARAM_IS_MANUAL = "isManual"
    const val HEADER_RELEASED = "headerreleased"
    const val REFRESH_STATE_CHANGE = "refreshstatechange"
    const val REFRESH_STATE_CHANGE_PARAM_STATE = "state"
    const val HEADER_OFFSET = "headeroffset"
    const val HEADER_OFFSET_PARAM_OFFSET_PERCENT = "offsetPercent"
    const val HEADER_OFFSET_PARAM_IS_DRAGGING = "isDragging"
    const val AUTO_REFRESH_DELAYED = 0
    const val AUTO_REFRESH_DURATION = 300
    const val AUTO_REFRESH_DRAG_RATE = 1.0f
    const val AUTO_REFRESH_ANIMATION_ONLY = false
    const val REFRESH_STATE_IDLE = 0
    const val REFRESH_STATE_DRAG_RELEASE = 1
    const val REFRESH_STATE_REFRESHING = 2
  }
  
  private var mEnableRefresh = true
  private var mManualRefresh = true

  constructor(context: LynxContext) : this(context, null)

  override fun createView(context: Context?): SmartRefreshLayout? {
    val ctx = context ?: return null
    // Note: createView is invoked when LynxBaseUI <init> and all fields are not init.
    mEnableRefresh = true
    mManualRefresh = true
    
    return object : SmartRefreshLayout(ctx) {
      override fun setRefreshContent(contentView: View): RefreshLayout {
        LLog.d(TAG, "setRefreshContent to SmartRefreshLayout: $contentView -> $this")
        mRefreshContent?.view?.let {
          LLog.d(TAG, "remove view from SmartRefreshLayout: $it")
          removeView(it)
        }
        mRefreshContent = object: RefreshContentWrapper(contentView) {
          override fun canRefresh(): Boolean {
            return mScrollableView?.let {
              mEnableRefresh && !(it.visibility == View.VISIBLE && it.canScrollVertically(-1))
            } ?: false
          }

          override fun onActionDown(event: MotionEvent?) {
            mContentView?.let {
              var point = PointF(event!!.x, event!!.y)
              point.offset(-mContentView.left.toFloat(), -mContentView.top.toFloat())
              mScrollableView = null
              findDeepScrollableView(mContentView, point)
              LLog.d(TAG, "finish search, point = $point, scrollableView = " +
                "$mScrollableView, contentView = $mContentView")
              mScrollableView = mScrollableView ?: mContentView
            }
          }

          private fun findDeepScrollableView(content: View, parentLocal: PointF) {
            content.let {
              val childLocal = PointF()
              if (it is ViewGroup) {
                for (i in it.childCount downTo 1) {
                  var child: View = it.getChildAt(i - 1)
                  if (isTouchPointInView(it, child, parentLocal, childLocal)) {
                    findDeepScrollableView(child, childLocal)
                  }
                }
                var isContentView = SmartUtil.isContentView(it)
                  || ((it is ViewPager) && (it as ViewPager).isVertical)
                  || (it is NestedScrollView)
                if (it !is androidx.viewpager.widget.ViewPager && isContentView && mScrollableView == null) {
                  // Here we should skip horizontal scroll view.
                  mScrollableView = if (it is HorizontalScrollView) null else it
                }
                val location = IntArray(2)
                it.getLocationOnScreen(location)
                val rect = Rect(location[0], location[1], location[0] + it.width, location[1] + it.height)
              }
            }
          }

          private fun isTouchPointInView(parent: View, 
                                         child: View, 
                                         parentLocal: PointF, 
                                         childLocal: PointF): Boolean {
            return when {
              child.visibility != View.VISIBLE -> false
              else -> {
                childLocal.x = parentLocal.x + parent.scrollX - child.left
                childLocal.y = parentLocal.y + parent.scrollY - child.top
                RectF(0F, 0F, child.width.toFloat(), child.height.toFloat())
                  .contains(childLocal.x, childLocal.y)
              }
            }
          }

          override fun canLoadMore(): Boolean = false
        }
        addView(contentView, childCount, LayoutParams(width, height))
        if (mAttachedToWindow) {
          mRefreshContent.setScrollBoundaryDecider(mScrollBoundaryDecider)
          mRefreshContent.setEnableLoadMoreWhenContentNotFull(mEnableLoadMoreWhenContentNotFull)
          mRefreshContent.setUpComponent(mKernel, null, null)
        }
        if (mRefreshHeader != null && mRefreshHeader.spinnerStyle.front) {
          super.bringChildToFront(mRefreshHeader.view)
        }
        if (mRefreshFooter != null && mRefreshFooter.spinnerStyle.front) {
          super.bringChildToFront(mRefreshFooter.view)
        }
        return this
      }
      
      override fun startFlingIfNeed(flingVelocity: Float): Boolean {
        val res = super.startFlingIfNeed(flingVelocity)
        mScroller?.let {
          LLog.i(TAG, "startFlingIfNeed: $flingVelocity, ${it.timePassed()}," +
            "${AnimationUtils.currentAnimationTimeMillis()}")
        }
        return res
      }

      override fun computeScroll() {
        try {
          super.computeScroll()
        } catch (e: IndexOutOfBoundsException) {
          mScroller?.let {
            LLog.e(TAG, "computeScroll: ${it.timePassed()}," +
              "${AnimationUtils.currentAnimationTimeMillis()}")
          }
        }
      }
    }.apply {
      LLog.i(TAG, "create SmartRefreshLayout: lynxUIRefresh=${this@LynxUIRefresh}, this=$this")
      setEnableRefresh(mEnableRefresh)
      setEnableLoadMore(false)
      setOnRefreshListener {
        LLog.d(TAG, "onRefresh")
        lynxContext?.eventEmitter?.sendCustomEvent(
          LynxDetailEvent(sign, REFRESH_STATE_CHANGE).apply {
            addDetail(REFRESH_STATE_CHANGE_PARAM_STATE, REFRESH_STATE_REFRESHING)
          }
        )
        lynxContext?.eventEmitter?.sendCustomEvent(
          LynxDetailEvent(sign, START_REFRESH).apply {
             addDetail(START_REFRESH_PARAM_IS_MANUAL, mManualRefresh)
          }
        )
        mManualRefresh = true
      }
      setOnMultiListener(object : SimpleMultiListener() {
        override fun onHeaderMoving(
          header: RefreshHeader?,
          isDragging: Boolean,
          percent: Float,
          offset: Int,
          headerHeight: Int,
          maxDragHeight: Int
        ) {
          super.onHeaderMoving(header, isDragging, percent, offset, headerHeight, maxDragHeight)
          lynxContext?.eventEmitter?.sendCustomEvent(
            LynxDetailEvent(sign, HEADER_OFFSET).apply {
              addDetail(HEADER_OFFSET_PARAM_OFFSET_PERCENT, percent)
              addDetail(HEADER_OFFSET_PARAM_IS_DRAGGING, isDragging)
            }
          )
        }
        
        override fun onHeaderReleased(header: RefreshHeader?, headerHeight: Int, maxDragHeight: Int) {
          super.onHeaderReleased(header, headerHeight, maxDragHeight)
          LLog.d(TAG, "onHeaderReleased")
          lynxContext?.eventEmitter?.sendCustomEvent(
            LynxDetailEvent(sign, REFRESH_STATE_CHANGE).apply {
              addDetail(REFRESH_STATE_CHANGE_PARAM_STATE, REFRESH_STATE_DRAG_RELEASE)
            }
          )
          lynxContext?.eventEmitter?.sendCustomEvent(
            LynxDetailEvent(sign, HEADER_RELEASED)
          )
        }

        override fun onStateChanged(refreshLayout: RefreshLayout, oldState: RefreshState, newState: RefreshState) {
          super.onStateChanged(refreshLayout, oldState, newState)
          if (newState == RefreshState.None) {
            LLog.d(TAG, "onStateChanged: refresh header idle")
            lynxContext?.eventEmitter?.sendCustomEvent(
              LynxDetailEvent(sign, REFRESH_STATE_CHANGE).apply {
                addDetail(REFRESH_STATE_CHANGE_PARAM_STATE, REFRESH_STATE_IDLE)
              }
            )
          }
        }
      })
    }
  }

  @LynxProp(name = "enable-refresh", defaultBoolean = true)
  fun setEnableRefresh(enable: Boolean) {
    LLog.d(TAG, "setEnableRefresh: $enable")
    this.mEnableRefresh = enable
    mView?.setEnableRefresh(enable)
  }

  @LynxUIMethod
  open fun finishRefresh(params: ReadableMap) {
    LLog.d(TAG, "finishRefresh")
    mView.finishRefresh()
  }

  @LynxUIMethod
  open fun autoStartRefresh(params: ReadableMap) {
    LLog.d(TAG, "autoStartRefresh")
    mManualRefresh = false
    mView.autoRefresh(AUTO_REFRESH_DELAYED, AUTO_REFRESH_DURATION, 
      AUTO_REFRESH_DRAG_RATE, AUTO_REFRESH_ANIMATION_ONLY)
  }

  override fun insertChild(child: LynxBaseUI?, index: Int) {
    LLog.i(TAG, "insertChild at index=$index, child=$child -> this=$this")
    onInsertChild(child, index)
    when (child) {
      is LynxUIRefreshHeader -> {
        mView.setRefreshHeader(child.view)
      }
      is LynxUI<*> -> {
        mView.setRefreshContent(child.view)
      }
      else -> {
        val error = LynxError(LynxSubErrorCode.E_COMPONENT_CUSTOM, 
          "LynxRefreshUI Only supports three types of children: refresh-header and non-flatten child ui as the refresh content.",
          "",  LynxError.LEVEL_ERROR
        )
        mContext?.handleLynxError(error)
      }
    }
  }

  override fun removeChild(child: LynxBaseUI?) {
    super.removeChild(child)
  }

  override fun canHaveFlattenChild(): Boolean = false

  override fun needCustomLayout(): Boolean = true

  override fun generateLayoutParams(childParams: ViewGroup.LayoutParams?): ViewGroup.LayoutParams {
    return SmartRefreshLayout.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.WRAP_CONTENT
    )
  }
}
