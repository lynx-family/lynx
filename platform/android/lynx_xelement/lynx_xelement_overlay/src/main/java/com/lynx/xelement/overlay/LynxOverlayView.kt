// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.app.Activity
import android.app.Dialog
import android.content.Context
import android.graphics.Color
import android.graphics.Rect
import android.os.Build
import android.view.*
import androidx.annotation.RequiresApi
import androidx.core.view.WindowCompat
import com.lynx.react.bridge.Dynamic
import com.lynx.react.bridge.ReadableType
import com.lynx.tasm.LynxError
import com.lynx.tasm.LynxSubErrorCode
import com.lynx.tasm.base.LLog
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.PropsConstants
import com.lynx.tasm.behavior.TouchEventDispatcher
import com.lynx.tasm.behavior.event.EventTarget
import com.lynx.tasm.behavior.event.EventTarget.EnableStatus
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.UIParent
import com.lynx.tasm.behavior.ui.list.UIList
import com.lynx.tasm.behavior.ui.scroll.UIScrollView
import com.lynx.tasm.behavior.ui.view.AndroidView
import com.lynx.tasm.behavior.ui.view.UIView
import com.lynx.tasm.event.EventsListener
import com.lynx.tasm.event.LynxDetailEvent
import com.lynx.tasm.utils.ContextUtils
import kotlin.math.abs

class LynxOverlayView(context: LynxContext, val proxy: LynxUIOverlay) : UIGroup<AndroidView>(context) {

    companion object {
        const val TAG = "overlay"
        const val EVENT_SHOW = "showoverlay"
        const val EVENT_DISMISS = "dismissoverlay"
        const val EVENT_REQUEST_CLOSE = "requestclose"
        const val EVENT_OVERLAY_MOVED = "overlaymoved"
        const val EVENT_OVERLAY_TOUCH = "overlaytouch"

    }

    private var mEventState = 0 //state 0-begin, 1-move, 2-up
    private var mVisible = false
    private var mStatusBarTranslucent = true
    private var mIsCutOutMode = true
    private var mEventsPassThrough = true
    private var mEventsPassThroughHasBeenSet = false
    private var mStatusBarTranslucentStyle: String = LynxUIOverlay.PROP_STATUS_BAR_TRANSLUCENT_STYLE_DARK
    private var mLazyInitContext = false

    private var mId: String? = null
    private var mNestScrollId: String? = null
    private var mNestedScrollView: LynxBaseUI? = null
    private var mCanNestedScroll = false
    private var mAlwaysShow = true
    private var mShouldOffsetBoundingRect = true
    private var mEnableOverlayMoved = false
    private var mEnableOverlayTouch = false
    private var mDialogDismissCleaned = true
    private var mPlatformEventRootActive = false
    private var mVelocityTracker: VelocityTracker? = null
    private var mLastX = 0.0f
    private var mLastY = 0.0f
    private var mIntercept : Boolean? = null

    private val mDialog = LynxOverlayDialog(context, this)
    private var mOverlayContainer = object : AndroidView(context) {
        override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
            // layout overlay when dialog decor view onLayout
            this@LynxOverlayView.layout()
        }
        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec))
            measureChildren()
        }
    }

    private val mOffsetDescendantRectToLynxView = intArrayOf(Int.MIN_VALUE, Int.MIN_VALUE)
    private val mSyncedPlatformEventRootOffset = intArrayOf(Int.MIN_VALUE, Int.MIN_VALUE)

    private var mEventDispatcher : TouchEventDispatcher? = null

    private var mObserver : ViewTreeObserver? = null
    private var mGlobalLayoutListener : ViewTreeObserver.OnGlobalLayoutListener? = null
    private var mScrollChangedListener : ViewTreeObserver.OnScrollChangedListener? = null
    private var mDrawListener : ViewTreeObserver.OnDrawListener? = null

    init {
        // do not dim the window behind
        mDialog?.window?.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND)
        mDialog?.window?.setDimAmount(0f)
        mOverlayContainer.addView(mView, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        mDialog?.setContentView(mOverlayContainer, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        mDialog?.setOnKeyListener { _, keyCode, event ->
            if (keyCode == KeyEvent.KEYCODE_BACK && event.action == KeyEvent.ACTION_DOWN) {
                requestDialogClose()
                true
            } else {
                false
            }
        }
        // By default, it remains consistent with not enabling edge to edge.
        setAdaptEdgeToEdge(false)
        mOverlayContainer.isClickable = true
        mOverlayContainer.isFocusable = true
        mOverlayContainer.isFocusableInTouchMode = true
        mEventDispatcher = TouchEventDispatcher(context.lynxUIOwner)
        translucentStatusBar(isStatusBarTranslucent())
    }

    private fun px2dip(pxValue: Float): Float {
        val scale = mContext.resources.displayMetrics.density
        return pxValue / scale + 0.5f
    }

    override fun setEvents(events: MutableMap<String, EventsListener>?) {
        super.setEvents(events)
        if (events == null) return
        //when enableOverlayMoved is true, it need to handle state and find nested scroll container
        // state 0-begin, 1-move, 2-up
        mEnableOverlayMoved = events.containsKey(EVENT_OVERLAY_MOVED)
        mEnableOverlayTouch = events.containsKey(EVENT_OVERLAY_TOUCH)
        if (mEnableOverlayMoved || mEnableOverlayTouch) {
            mVelocityTracker = VelocityTracker.obtain()
            mDialog?.setTouchListener(object : LynxOverlayDialog.TouchEventListener {
                override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
                    //track velocity if bindOverLayMoved, when vx is positive, it means move right, when vy is positive, it means move down
                    mVelocityTracker?.addMovement(ev)
                    when (ev.action) {
                        MotionEvent.ACTION_DOWN -> {
                            mEventState = 0
                            if(mEnableOverlayMoved) {
                                mNestedScrollView = null
                                mCanNestedScroll = canNestedScroll(hitTest(ev.x, ev.y))
                                mIntercept = null
                                mLastX = ev.x
                                mLastY = ev.y
                            }
                        }
                        MotionEvent.ACTION_MOVE -> {
                            mEventState = 1
                            if (mEnableOverlayMoved) {
                                if (mIntercept == null && ((abs(ev.y - mLastY)) > 0.0f || abs(ev.x - mLastX) > 0.0f)) {
                                    mIntercept = if (mNestedScrollView != null) {
                                        !mCanNestedScroll && ((ev.y - mLastY) > 0.0f)
                                    } else {
                                        !mCanNestedScroll && (abs(ev.y - mLastY) > abs(ev.x - mLastX))
                                    }
                                    mLastY = ev.y
                                }
                            }
                        }
                        MotionEvent.ACTION_UP -> mEventState = 2
                    }
                    mVelocityTracker?.computeCurrentVelocity(1000)

                    if (mEnableOverlayMoved && mIntercept != false) {
                        sendOverlayMovedEvent(
                            EVENT_OVERLAY_MOVED,
                            px2dip(ev.x), px2dip(ev.y), mVelocityTracker?.xVelocity ?: 0.0f,
                            mVelocityTracker?.yVelocity ?: 0.0f, mEventState
                        )
                    }
                    if(mEnableOverlayTouch) {
                        sendOverlayMovedEvent(
                            EVENT_OVERLAY_TOUCH,
                            px2dip(ev.rawX), px2dip(ev.rawY), mVelocityTracker?.xVelocity ?: 0.0f,
                            mVelocityTracker?.yVelocity ?: 0.0f, mEventState
                        )

                    }

                    return mIntercept ?: false
                }
            })
        }
    }

    private fun canNestedScroll(target: EventTarget?) : Boolean {
        if (target == null || target !is LynxBaseUI || target is LynxOverlayView) return false
        if (mNestScrollId.isNullOrEmpty()) return false
        if (mNestedScrollView != null) {
            return canScrollContainerNestedScroll(mNestedScrollView)
        }
        if (target !is UIGroup<*>) {
            canNestedScroll(target.parent())
        }
        if (mNestScrollId == target.idSelector) {
            mNestedScrollView = target
            return canScrollContainerNestedScroll(mNestedScrollView)
        }
        return canNestedScroll(target.parent())
    }

    private fun canScrollContainerNestedScroll(view: LynxBaseUI?) : Boolean {
        if (view is UIScrollView && !view.view.isHorizontal) {
            return view.view.realScrollY > 0
        } else if (view is UIList && view.isVertical) {
            return view.view.computeVerticalScrollOffset() > 0
        } else if (view is LynxUI<*>) {
            return view.view.scrollY > 0
        }
        return false
    }

    override fun createView(context: Context?): AndroidView = AndroidView(context)

    override fun getTouchEventDispatcher() : TouchEventDispatcher? {return mEventDispatcher}

    override fun eventThrough(x: Float, y: Float): Boolean {
        // If mEventThrough == Enable, let the res be true. Otherwise, return false.
        var isEventThrough: Boolean = false
        if (mEventThrough == EventTarget.EnableStatus.Enable) {
            isEventThrough = true;
        }

        if (mEventThroughActiveRegions == null) {
            return isEventThrough
        }

        var isHitEventThroughActiveRegions = false
        for (i in mEventThroughActiveRegions.indices) {
            val region = mEventThroughActiveRegions[i]
            if (region != null && region.size == 4) {
                val left = region[0].convertToDevicePx(width.toFloat())
                val top = region[1].convertToDevicePx(height.toFloat())
                val right = left + region[2].convertToDevicePx(width.toFloat())
                val bottom = top + region[3].convertToDevicePx(height.toFloat())
                isHitEventThroughActiveRegions =
                    x >= left && x < right && y >= top && y < bottom
                if (isHitEventThroughActiveRegions) {
                    LLog.i(TAG, "hit the event through active regions!")
                    break
                }
            }
        }
        return if (isHitEventThroughActiveRegions) isEventThrough else !isEventThrough
    }

    override fun ignoreFocus(): Boolean {
        if (mIgnoreFocus == EventTarget.EnableStatus.Enable) {
            return true
        }
        return false;
    }

    override fun pointerEvents(): EventTarget.PointerEventsValue {
        if (mPointerEvents == EventTarget.PointerEventsValue.None) {
            return EventTarget.PointerEventsValue.None
        }
        return EventTarget.PointerEventsValue.Auto
    }

    @LynxProp(name = LynxUIOverlay.PROP_VISIBLE)
    fun setVisible(visible: Dynamic) {
        when (visible.type) {
            ReadableType.String -> {
                mVisible = visible.asString()!!.toBoolean()
            }
            ReadableType.Boolean -> {
                mVisible = visible.asBoolean()
            }
        }

        if (mVisible) {
            show()
        } else {
            hide()
        }
    }

    /**
     * @name: android-full-screen
     * @description: On the Android system, when the bottom navigation bar is opened,
     * if the content is at the bottom of the screen, it will block the content in overlay,
     * so use this attribute to hide the bottom navigation bar
     * @category: different
     * @standardAction: keep
     * @supportVersion: 2.12
     **/
    @LynxProp(name = LynxUIOverlay.PROP_FULL_SCREEN)
    fun setFullScreen(fullScreen: Boolean) {
        if(fullScreen){
            val flag = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or mDialog?.window?.decorView?.systemUiVisibility!!)
            mDialog?.window?.decorView?.systemUiVisibility = flag
        }
    }

    /**
     * @name: android-hide-navigation-bar
     * @description: On Android, when you need hide the navigation bar, you can set this property to true.
     * @category: different
     * @standardAction: keep
     * @supportVersion: 3.4
     **/
    @LynxProp(name = LynxUIOverlay.PROP_ANDROID_HIDE_NAVIGATION_BAR)
    fun hideNavBar(hide: Boolean) {
        if (hide) {
            val flag = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or mDialog?.window?.decorView?.systemUiVisibility!!)
            mDialog?.window?.decorView?.systemUiVisibility = flag
        } else {
            mDialog?.window?.decorView?.systemUiVisibility = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        }
    }


    /**
     * @name: android-lazy-init-context
     * @description: In Android, if a container has preloading or background tasks for Lynx, passing the context during the initialization of a page isn't necessarily the final context for displaying the page. For overlay components, you need to make additional adaptations for this situation. When actually displaying the overlay, you should change the context held by the overlay to the context of the current page. This is necessary to ensure that the overlay can display correctly on the current page.
     * @category: different
     * @standardAction: keep
     * @supportVersion: 2.12
     **/
    @LynxProp(name = LynxUIOverlay.PROP_ANDROID_LAZY_INIT_CONTEXT)
    fun setLazyInitContext(lazyInitContext: Boolean) {
        mLazyInitContext = lazyInitContext
    }

    @LynxProp(name = LynxUIOverlay.PROP_STATUS_BAR_TRANSLUCENT)
    fun setStatusBarTranslucent(statusBarTranslucent: Dynamic) {
        when (statusBarTranslucent.type) {
            ReadableType.String -> {
                mStatusBarTranslucent = statusBarTranslucent.asString()!!.toBoolean()
            }
            ReadableType.Boolean -> {
                mStatusBarTranslucent = statusBarTranslucent.asBoolean()
            }
        }
        translucentStatusBar(isStatusBarTranslucent())
    }

    @LynxProp(name = LynxUIOverlay.CUT_OUT_MODE)
    fun setCutOutMode(isCutOut: Boolean) {
        mIsCutOutMode = isCutOut
        mDialog?.window?.attributes?.apply {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                layoutInDisplayCutoutMode = if (isCutOut) {
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
                } else {
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER
                }
            }
        }
    }

    @LynxProp(name = LynxUIOverlay.PROP_NEST_SCROLL)
    fun setNestScroll(id: String) {
        mNestScrollId = id
        mNestedScrollView = null
    }

    fun isStatusBarTranslucent(): Boolean {
        return (mStatusBarTranslucent && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
    }

    fun isCutOutMode(): Boolean {
        return mIsCutOutMode
    }

    @LynxProp(name = LynxUIOverlay.PROP_EVENTS_PASS_THROUGH)
    fun setEventsPassThrough(eventsPassThrough: Dynamic) {
        mEventsPassThroughHasBeenSet = true
        when (eventsPassThrough.type) {
            ReadableType.String -> {
                mEventsPassThrough = eventsPassThrough.asString()!!.toBoolean()
            }
            ReadableType.Boolean -> {
                mEventsPassThrough = eventsPassThrough.asBoolean()
            }
        }
    }

    @RequiresApi(Build.VERSION_CODES.KITKAT)
    @LynxProp(name = LynxUIOverlay.PROP_STATUS_BAR_TRANSLUCENT_STYLE)
    fun setStatusBarTranslucentStyle(value: String?) {
        mStatusBarTranslucentStyle = value ?: LynxUIOverlay.PROP_STATUS_BAR_TRANSLUCENT_STYLE_DARK
        translucentStatusBar(isStatusBarTranslucent())
    }

    @LynxProp(name = LynxUIOverlay.PROP_ALWAYS_SHOW)
    fun setAlwaysShow(boolean: Boolean) {
        mAlwaysShow = boolean
    }

    @LynxProp(name = LynxUIOverlay.PROP_LEVEL, defaultInt = 1)
    fun setLevel(level: Int) {
        if (level == mDialog.level) {
            return
        }
        if (mDialog.isShowing) {
            val error = LynxError(
                LynxSubErrorCode.E_COMPONENT_CUSTOM,
                "Overlay set Level error!",
                "The visible attribute needs to default to false. After creation, change visible to true at the appropriate time",
                LynxError.LEVEL_ERROR
            )
            error.isLogBoxOnly = true
            lynxContext.handleLynxError(error)
            return
        }

        when(level) {
            1 -> setDialogType(WindowManager.LayoutParams.TYPE_APPLICATION)
            2 -> setDialogType(WindowManager.LayoutParams.FIRST_APPLICATION_WINDOW)
            3 -> setDialogType(WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL)
            4 -> setDialogType(WindowManager.LayoutParams.TYPE_APPLICATION_PANEL)
            else -> setDialogType(WindowManager.LayoutParams.TYPE_APPLICATION)
        }
        mDialog.level = level
    }

    /**
     * @name: android-container-popup-tag
     * @description On Android, when the overlay shown in popup container, event pass needs to specify the corresponding DialogFragment Tag to correctly dispatch the event to the corresponding popup container.
     * @Android
     * @defaultValue ""
     */
    @LynxProp(name = LynxUIOverlay.PROP_CONTAINER_POPUP_TAG)
    fun setContainerPopupTag(tag: String) {
        mDialog.containerPopupTag = tag
    }

    /**
     * @name: android-adapt-edge-to-edge
     * @description: On Android, controls whether the window should adapt to edge-to-edge display. This property only takes effect on Android 15 (API level 35) and above. When set to true, the window content will extend into the system bar areas.
     * @category: different
     * @standardAction: keep
     * @supportVersion: 3.6
     **/
    @LynxProp(name = LynxUIOverlay.PROP_ADAPT_EDGE_TO_EDGE)
    fun setAdaptEdgeToEdge(adapt: Boolean) {
      window?.let {
        //Build.VERSION_CODES.VANILLA_ICE_CREAM
        if (Build.VERSION.SDK_INT >= 35)
          WindowCompat.setDecorFitsSystemWindows(it, !adapt)
      }
    }
  
    /**
     * @name: android-set-soft-input-mode
     * @description: Set the soft keyboard mode to control the input state after the keyboard pops up [nothing | pan | resize | unspecified]
     * @category: different
     * @standardAction: keep
     * @supportVersion: 2.7
     **/
    @LynxProp(name = "android-set-soft-input-mode")
    fun setAndroidSetSoftInputMode(value: String) {
        if ("unspecified".equals(value)) {
            mDialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_UNSPECIFIED)
        } else if ("nothing".equals(value)) {
            mDialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING)
        } else if ("pan".equals(value)) {
            mDialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        } else if ("resize".equals(value)) {
            mDialog.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        }
    }

    /**
     * @name: android-native-event-pass
     * @description: In Android, when event-pass-through is set, events are
     *managed by Lynx. It will pass through events that the current overlay does
     *not consume to the underlying overlay or the entire page's activity.
     *However, it will ignore native views within it, including popups on the
     *container side. Therefore, by setting android-native-event-pass to true,
     *you can pass through all native events, allowing the system's
     *WindowManager to manage them and continue dispatching them to the next
     *layer. This is equivalent to only displaying them without interaction; all
     *native events will be passed through.
     * @category: different
     * @standardAction: keep
     * @supportVersion: 2.12
     **/
    @LynxProp(name = "android-native-event-pass")
    fun setNativeEventPass(enable : Boolean) {
        if (enable) {
            mDialog.window?.addFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE)
        } else {
            mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE)
        }
    }

    @LynxProp(name = PropsConstants.IGNORE_FOCUS)
    override fun setIgnoreFocus(ignoreFocus: Dynamic?) {
        super.setIgnoreFocus(ignoreFocus)
        var isIgnoreFocus: Boolean = mIgnoreFocus == EnableStatus.Enable
        mOverlayContainer.isFocusable = !isIgnoreFocus
        mOverlayContainer.isFocusableInTouchMode = !isIgnoreFocus
        if (isIgnoreFocus) {
            mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM)
            mDialog.window?.setFlags(
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
            )
        } else {
            mDialog.window?.clearFlags(
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
            )
        }
    }

    private fun setDialogType(type: Int) {
        mDialog.window?.setType(type)
    }

    override fun requestLayout() {
        super.requestLayout()
        if (proxy.transitionAnimator != null || proxy.enableLayoutAnimation()) {
            mOverlayContainer.invalidate()
        }
    }

    private fun translucentStatusBar(translucent: Boolean) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (translucent) {
                mDialog.window?.addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR)
                mDialog.window?.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
                mDialog.window?.statusBarColor = Color.TRANSPARENT
            } else {
                mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN)
                mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR)
                mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
            }
            val visibilityFlag = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                    && mStatusBarTranslucentStyle == LynxUIOverlay.PROP_STATUS_BAR_TRANSLUCENT_STYLE_LITE) {
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR or mDialog?.window?.decorView?.systemUiVisibility!!
            } else {
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_LAYOUT_STABLE or mDialog?.window?.decorView?.systemUiVisibility!!
            }
            if (translucent) {
                mDialog.window?.decorView?.systemUiVisibility = visibilityFlag
            } else {
                mDialog.window?.decorView?.systemUiVisibility = View.STATUS_BAR_VISIBLE
            }
        } else {
            if (translucent) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                    mDialog.window?.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS)
                }
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                    mDialog.window?.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS)
                }
            }
        }
    }

    private fun sendEventWithoutParam(eventName: String) {
        lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(sign, eventName)
        )
    }

    private fun sendShowOverlayEvent(errorCode: Int, context: Context?) {
        var errorMsg = ""
        when (errorCode) {
            LynxOverlayDialog.ERROR_CODE_VALID -> errorMsg = "show success, context is $context"
            LynxOverlayDialog.ERROR_CODE_IS_FINISHING -> errorMsg = "context is finishing, context is $context"
            LynxOverlayDialog.ERROR_CODE_IS_DESTROYED -> errorMsg = "context is destroyed, context is $context"
            LynxOverlayDialog.ERROR_CODE_NOT_ACTIVITY_CONTEXT -> errorMsg = "context is not activity, context is $context"
        }
        sendOverlayErrorEvent(errorCode, errorMsg)
    }

    private fun sendOverlayErrorEvent(errorCode: Int, errorMsg: String) {
        lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(sign, EVENT_SHOW).apply {
                addDetail("errorCode", errorCode)
                addDetail("errorMsg", errorMsg)
            }
        )
    }

    private fun sendOverlayMovedEvent(eventName: String, x: Float, y: Float, vx: Float, vy: Float, state: Int) {
        lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(sign, eventName).apply {
                addDetail("x", x)
                addDetail("y", y)
                addDetail("vx", vx)
                addDetail("vy", vy)
                addDetail("state", mEventState)
            }
        )
    }

    private fun requestDialogClose() {
        sendEventWithoutParam(EVENT_REQUEST_CLOSE)
    }

    private fun setPlatformEventRootActive(active: Boolean) {
        if (active) {
            syncPlatformEventRootOffset()
        } else {
            mSyncedPlatformEventRootOffset[0] = Int.MIN_VALUE
            mSyncedPlatformEventRootOffset[1] = Int.MIN_VALUE
        }
        lynxContext.setPlatformEventRootActive(sign, active)
        mPlatformEventRootActive = active
    }

    private fun cleanupDialogDismiss(sendDismissEvent: Boolean) {
        if (mDialogDismissCleaned) {
            return
        }
        mDialogDismissCleaned = true
        if (mPlatformEventRootActive) {
            setPlatformEventRootActive(false)
        }
        if (sendDismissEvent) {
            sendEventWithoutParam(EVENT_DISMISS)
        }
        LynxOverlayManager.removeGlobalId(mId)
        mObserver?.removeOnGlobalLayoutListener(mGlobalLayoutListener)
        mObserver?.removeOnScrollChangedListener(mScrollChangedListener)
        mObserver?.removeOnDrawListener(mDrawListener)
        mObserver = null
    }

    private fun show() {
        val activity = ContextUtils.getActivity(lynxContext)
        if (mLazyInitContext) {
            changeCurrentContextToDialog(mDialog, activity)
        }
        if (activity != null) {
            if (!(activity.isFinishing) && !LynxOverlayManager.containsGlobalId(mId)) {
                try {
                    mId = LynxOverlayManager.addGlobalId(mDialog)
                    val errorCode = mDialog.checkContextErrorCode(activity)
                    if (errorCode >= 0) {
                        mDialogDismissCleaned = false
                        mDialog.setOnDismissListener {
                            cleanupDialogDismiss(true)
                        }
                        mDialog.setOnCancelListener {
                            cleanupDialogDismiss(true)
                        }
                        mDialog.show()
                        if (mDialog.isShowing) {
                            setPlatformEventRootActive(true)
                        }
                    }
                    sendShowOverlayEvent(errorCode, activity)
                    mObserver = mOverlayContainer?.viewTreeObserver
                    mGlobalLayoutListener = ViewTreeObserver.OnGlobalLayoutListener { lynxContext?.exposure?.requestCheckUI() }
                    mObserver?.addOnGlobalLayoutListener(mGlobalLayoutListener)
                    mScrollChangedListener = ViewTreeObserver.OnScrollChangedListener { lynxContext?.exposure?.requestCheckUI() }
                    mObserver?.addOnScrollChangedListener(mScrollChangedListener)
                    mDrawListener = ViewTreeObserver.OnDrawListener { lynxContext?.exposure?.requestCheckUI() }
                    mObserver?.addOnDrawListener(mDrawListener)
                } catch (e: WindowManager.BadTokenException) {
                    LLog.w(TAG, e.toString())
                } catch (e: RuntimeException) {
                    LLog.w(TAG, e.toString())
                }
            }
        } else {
            sendShowOverlayEvent(LynxOverlayDialog.ERROR_CODE_NOT_ACTIVITY_CONTEXT, lynxContext?.baseContext)
        }
    }

    private fun changeCurrentContextToDialog(dialog: LynxOverlayDialog, activity: Activity?) {
        try {
            val contextField = Dialog::class.java.getDeclaredField("mContext")
            contextField.isAccessible = true
            contextField.set(dialog, activity)
            contextField.isAccessible = false
        } catch (e: Exception) {
            // need to Handle any exceptions that might occur during reflection.
            LLog.e(TAG, e.toString())
        }
    }

    private fun hide() {
        if (mDialog.isShowing) {
            try {
                cleanupDialogDismiss(true)
                mDialog.dismiss()
            } catch (e: WindowManager.BadTokenException) {
                LLog.w(TAG, e.toString())
            } catch (e: RuntimeException) {
                LLog.w(TAG, e.toString())
            }
        }
    }

    fun needHandleEvent(x: Float, y: Float): Boolean {
        // when the overlay is invisible, will not handle any events
        if (!mVisible) {
            return false
        }

        if (!mEventsPassThroughHasBeenSet) {
            if (!eventThrough(x, y)) {
                return true
            }
        } else if (!mEventsPassThrough && !eventThrough(x, y)) {
            // the overlay will handle all events in the container, if mEventsPassThrough = false && eventThrough() = false
            return true
        }

        // the overlay will only handle children's events
        mChildren.forEach { ui ->
            if (getTransLeft() + ui.left + ui.translationX < x
                && getTransLeft() + ui.left.toFloat() + ui.translationX + ui.width.toFloat() > x
                && getTransTop() + ui.top + ui.translationY < y
                && getTransTop()  + ui.top.toFloat() + ui.translationY + ui.height.toFloat() > y) {
                return true
            }
        }

        return false
    }


    fun getTransLeft() = left

    fun getTransTop() = top

    fun isViewVisible(): Boolean {
        return mVisible
    }

    override fun setParent(parent: UIParent?) {
        super.setParent(parent)
        // if parent is null, it means the overlay will be removed
        if (parent == null) {
            hide()
        } else {
            if (mVisible) {
                // when re-attach overlay, show component when visible property is true
                show()
            }
        }
    }

    override fun onAttach() {
        super.onAttach()
        if (mEnableOverlayMoved) {
            mVelocityTracker = VelocityTracker.obtain()
        }
    }

    override fun onDetach() {
        super.onDetach()
        if (mEnableOverlayMoved) {
            try {
                mVelocityTracker?.recycle()
            } catch (t: Throwable) {
                LLog.e(TAG, t.toString())
            }
            mVelocityTracker = null
        }
        if(!mAlwaysShow) {
            hide()
        }
    }

    override fun isUserInteractionEnabled(): Boolean {
        return false
    }

    override fun destroy() {
        if (mDialog.isShowing) {
            try {
                cleanupDialogDismiss(false)
                mDialog.dismiss()
            } catch (e: WindowManager.BadTokenException) {
                LLog.w(TAG, e.toString())
            } catch (e: RuntimeException) {
                LLog.w(TAG, e.toString())
            }
        }
        super.destroy()
    }

    /* The overlayView is in another window, and the previously obtained coordinates need to be offset
     * based on the relative positions of LynxView and Overlay
     */
    override fun getBoundingClientRect(): Rect {
        if (mShouldOffsetBoundingRect && mOffsetDescendantRectToLynxView[0] == Int.MIN_VALUE) {
            updateOffsetDescendantRectToLynxView()
        }
        return super.getBoundingClientRect()
    }

    override fun onInsertChild(child: LynxBaseUI?, index: Int) {
        if (mShouldOffsetBoundingRect && mOffsetDescendantRectToLynxView[0] == Int.MIN_VALUE) {
            updateOffsetDescendantRectToLynxView()
        }
        super.onInsertChild(child, index)
    }

    private fun updateOffsetDescendantRectToLynxView() {
        val uiBodyView = lynxContext.uiBody.bodyView
        val uiBodyViewLocation = intArrayOf(0, 0)
        val overlayViewLocation = intArrayOf(0, 0)
        uiBodyView.getLocationOnScreen(uiBodyViewLocation)
        this.mView.getLocationOnScreen(overlayViewLocation)
        mOffsetDescendantRectToLynxView[0] = overlayViewLocation[0] - uiBodyViewLocation[0]
        mOffsetDescendantRectToLynxView[1] = overlayViewLocation[1] - uiBodyViewLocation[1]
    }

    private fun syncPlatformEventRootOffset() {
        if (mShouldOffsetBoundingRect) {
            updateOffsetDescendantRectToLynxView()
        }
        val offsetX = if (mOffsetDescendantRectToLynxView[0] == Int.MIN_VALUE) {
            0
        } else {
            mOffsetDescendantRectToLynxView[0]
        }
        val offsetY = if (mOffsetDescendantRectToLynxView[1] == Int.MIN_VALUE) {
            0
        } else {
            mOffsetDescendantRectToLynxView[1]
        }
        if (mSyncedPlatformEventRootOffset[0] == offsetX &&
            mSyncedPlatformEventRootOffset[1] == offsetY) {
            return
        }
        mSyncedPlatformEventRootOffset[0] = offsetX
        mSyncedPlatformEventRootOffset[1] = offsetY
        lynxContext.setPlatformEventRootOffset(
            sign,
            offsetX.toFloat(),
            offsetY.toFloat()
        )
    }

    override fun layout() {
        super.layout()
        if (mShouldOffsetBoundingRect && mDialog.isShowing) {
            syncPlatformEventRootOffset()
        } else if (mShouldOffsetBoundingRect) {
            updateOffsetDescendantRectToLynxView()
        }
    }

    override fun getOffsetDescendantRectToLynxView(): IntArray {
        return mOffsetDescendantRectToLynxView
    }

    override fun getWindow(): Window? {
        return mDialog?.window
    }

    override fun isOverlay(): Boolean {
        return true
    }
}
