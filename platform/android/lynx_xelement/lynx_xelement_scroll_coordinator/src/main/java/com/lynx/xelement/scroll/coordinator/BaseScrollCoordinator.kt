// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.scroll.coordinator

import android.content.Context
import android.view.ViewGroup
import androidx.appcompat.widget.Toolbar
import androidx.coordinatorlayout.widget.CoordinatorLayout
import com.google.android.material.appbar.AppBarLayout
import com.google.android.material.appbar.CollapsingToolbarLayout
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.view.UISimpleView
import com.lynx.tasm.event.EventsListener
import com.lynx.tasm.event.LynxDetailEvent
import kotlin.math.abs

abstract class BaseScrollCoordinator<
  K : BaseScrollCoordinatorAppBarLayout,
  T : ScrollCoordinatorToolbarLayout<K>,
>(
  context: LynxContext?,
  params: Any?,
) : UISimpleView<T>(context, params) {
  protected open lateinit var coordinatorLayout: T
  protected var enableOffsetEvent = false
  protected var lastSentOffset = 0f
  protected var offsetGranularity = 0.01f
  private var lastOffset = 0

  companion object {
    const val TAG = "ScrollCoordinator"
    const val OFFSET_EVENT = "offset"
  }

  constructor(context: LynxContext?) : this(context, null)

  abstract override fun createView(context: Context?): T?

  protected fun initializeDefaults(context: Context) {
    coordinatorLayout.layoutParams =
      ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
    bindOffsetChangedListener()
  }

  protected fun isCoordinatorLayoutInitialized(): Boolean {
    return this::coordinatorLayout.isInitialized
  }

  protected open fun isOffsetSupportHeight(): Boolean = false

  protected open fun getHeaderTapSlopSize(): Int = 0

  fun getYOffset(): Int {
    return abs(lastOffset)
  }

  override fun getScrollY(): Int {
    return abs(lastOffset)
  }

  protected fun bindOffsetChangedListener() {
    coordinatorLayout.getAppBarLayout().addOnOffsetChangedListener(
      object : AppBarLayout.OnOffsetChangedListener {
        override fun onOffsetChanged(layout: AppBarLayout?, offset: Int) {
          onAppBarOffsetChanged(layout, offset)
          if (abs(lastOffset - offset) > getHeaderTapSlopSize()) {
            recognizeGesturere()
            lastOffset = offset
          }
          lynxContext.intersectionObserverManager?.notifyObservers()

          if (!enableOffsetEvent) {
            return
          }

          val collapsingHeight = coordinatorLayout.getCollapsingToolbar().height
          val toolbarHeight = coordinatorLayout.getPinnedToolbar().height
          val totalOffsetHeight = collapsingHeight - toolbarHeight
          if (totalOffsetHeight == 0) {
            return
          }

          LLog.d(TAG, "onOffsetChanged: $offset, height = $totalOffsetHeight ")
          val currentOffset = abs(offset.toFloat()) / totalOffsetHeight
          if ((abs(lastSentOffset - currentOffset) < offsetGranularity && currentOffset < 1.0f)
            || lastSentOffset == currentOffset
          ) {
            return
          }
          lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(sign, OFFSET_EVENT).apply {
              if (isOffsetSupportHeight()) {
                addDetail("offset", pxToDip(abs(offset).toFloat()))
                addDetail(
                  "height",
                  pxToDip(totalOffsetHeight.toFloat()),
                )
              } else {
                addDetail("offset", String.format(java.util.Locale.ENGLISH, "%.5f", currentOffset))
              }
            },
          )
          lastSentOffset = currentOffset
        }
      },
    )
  }

  abstract fun onAppBarOffsetChanged(layout: AppBarLayout?, offset: Int)

  abstract fun updateAuxiliaryViewsOnTree()

  abstract override fun insertChild(child: LynxBaseUI, index: Int)

  abstract override fun removeChild(child: LynxBaseUI)

  override fun setEvents(events: MutableMap<String, EventsListener>?) {
    super.setEvents(events)
    LLog.d(TAG, "events: $events")
    if (events != null) {
      enableOffsetEvent = events.containsKey(OFFSET_EVENT)
    }
  }

  override fun generateLayoutParams(childParams: ViewGroup.LayoutParams?): ViewGroup.LayoutParams {
    if (childParams != null) {
      if (childParams.width == ViewGroup.LayoutParams.MATCH_PARENT
        && childParams.height == ViewGroup.LayoutParams.WRAP_CONTENT
      ) {
        return childParams
      }
      childParams.width = ViewGroup.LayoutParams.MATCH_PARENT
      childParams.height = ViewGroup.LayoutParams.WRAP_CONTENT
      when (childParams) {
        is CollapsingToolbarLayout.LayoutParams -> {
          return if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT) {
            CollapsingToolbarLayout.LayoutParams(childParams)
          } else {
            CollapsingToolbarLayout.LayoutParams(ViewGroup.LayoutParams(childParams))
          }
        }
        is CoordinatorLayout.LayoutParams -> {
          return CoordinatorLayout.LayoutParams(childParams).apply {
            behavior = AppBarLayout.ScrollingViewBehavior()
          }
        }
        is Toolbar.LayoutParams -> return Toolbar.LayoutParams(childParams)
      }
    }

    return CoordinatorLayout.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.WRAP_CONTENT,
    ).apply {
      behavior = AppBarLayout.ScrollingViewBehavior()
    }
  }

  override fun needCustomLayout(): Boolean = true

  private val offsetWithoutAnimationMethod by lazy {
    val appBarLayoutBaseBehavior = AppBarLayout.Behavior::class.java.superclass
    var method: java.lang.reflect.Method? = null
    try {
      method =
        appBarLayoutBaseBehavior?.getDeclaredMethod(
          "setHeaderTopBottomOffset",
          CoordinatorLayout::class.java,
          AppBarLayout::class.java,
          Int::class.java,
          Int::class.java,
          Int::class.java,
        )
    } catch (e: Exception) {
      LLog.e(TAG, "init offsetWithoutAnimationMethod error " + e.message)
    }
    method?.isAccessible = true
    return@lazy method
  }

  private val animateOffsetMethod by lazy {
    val appBarLayoutBaseBehavior = AppBarLayout.Behavior::class.java.superclass
    var method: java.lang.reflect.Method? = null
    try {
      method =
        appBarLayoutBaseBehavior?.getDeclaredMethod(
          "animateOffsetTo",
          CoordinatorLayout::class.java,
          AppBarLayout::class.java,
          Int::class.java,
          Float::class.java,
        )
    } catch (e: Exception) {
      LLog.e(TAG, "init animateOffsetMethod error " + e.message)
    }
    method?.isAccessible = true
    return@lazy method
  }

  protected fun animateToOffset(offsetPercent: Double = -1.0, offsetPx: Int = -1) {
    val appBarLayout = coordinatorLayout.getAppBarLayout()
    val behavior = (appBarLayout.layoutParams as CoordinatorLayout.LayoutParams).behavior
    if (behavior is AppBarLayout.Behavior) {
      try {
        val offset =
          (if (offsetPx >= 0) {
            -offsetPx
          } else {
            -((appBarLayout.totalScrollRange * (1 - offsetPercent)).toInt())
          }).coerceAtLeast(-appBarLayout.totalScrollRange)

        animateOffsetMethod?.invoke(behavior, coordinatorLayout, appBarLayout, offset, 0)
      } catch (e: Exception) {
        LLog.e(TAG, "invoke animateOffsetMethod error " + e.message)
      }
    }
  }

  protected fun setOffsetWithoutAnimation(offsetPercent: Double = -1.0, offsetPx: Int = -1) {
    val appBarLayout = coordinatorLayout.getAppBarLayout()
    val behavior = (appBarLayout.layoutParams as CoordinatorLayout.LayoutParams).behavior
    if (behavior is AppBarLayout.Behavior) {
      try {
        val offset =
          (if (offsetPx >= 0) {
            -offsetPx
          } else {
            -((appBarLayout.totalScrollRange * (1 - offsetPercent)).toInt())
          }).coerceAtLeast(-appBarLayout.totalScrollRange)

        offsetWithoutAnimationMethod?.invoke(
          behavior,
          coordinatorLayout,
          appBarLayout,
          offset,
          Int.MIN_VALUE,
          Int.MAX_VALUE,
        )
      } catch (e: Exception) {
        LLog.e(TAG, "invoke setOffsetWithoutAnimation error " + e.message)
      }
    }
  }

  @LynxProp(name = "scroll-enable", defaultBoolean = true)
  fun setScrollEnable(enable: Boolean) {
    coordinatorLayout.findViewById<K>(R.id.app_bar_layout).setScrollEnabled(enable)
  }

  @LynxProp(name = "granularity", defaultFloat = 0.01f)
  fun setGranularity(value: Float) {
    offsetGranularity = value
  }

  @LynxProp(name = "compat-container-popup", defaultBoolean = false)
  fun setCompatContainerPopup(enable: Boolean) {
    coordinatorLayout.setCompatContainerPopupEnabled(enable)
  }

  private fun pxToDip(px: Float): Int {
    val density = mContext.resources.displayMetrics.density
    return (px / density + 0.5f).toInt()
  }
}
