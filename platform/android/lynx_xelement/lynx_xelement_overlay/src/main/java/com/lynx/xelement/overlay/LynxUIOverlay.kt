// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.content.Context
import android.graphics.Rect
import android.view.View
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.StylesDiffMap
import com.lynx.tasm.behavior.TouchEventDispatcher
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.LynxViewVisibleHelper
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.UIParent
import com.lynx.tasm.behavior.ui.view.AndroidView
import com.lynx.tasm.event.EventsListener
import com.lynx.tasm.utils.ContextUtils
import com.lynx.tasm.utils.DisplayMetricsHolder

//This is proxy in lynx layout node, real ui node is LynxUIOverlay
open class LynxUIOverlay(context: LynxContext, params: Any?): UIGroup<AndroidView>(context, params),LynxViewVisibleHelper {

    companion object {
        const val PROP_VISIBLE = "visible"
        const val PROP_STATUS_BAR_TRANSLUCENT = "status-bar-translucent"
        const val PROP_EVENTS_PASS_THROUGH = "events-pass-through"
        const val CUT_OUT_MODE = "cut-out-mode"
        const val PROP_STATUS_BAR_TRANSLUCENT_STYLE = "status-bar-translucent-style"
        const val PROP_STATUS_BAR_TRANSLUCENT_STYLE_LITE = "lite"
        const val PROP_STATUS_BAR_TRANSLUCENT_STYLE_DARK = "dark"
        const val PROP_ALWAYS_SHOW = "always-show"
        const val PROP_NEST_SCROLL = "nest-scroll"
        const val PROP_LEVEL = "level"
        const val PROP_ANDROID_LAZY_INIT_CONTEXT = "android-lazy-init-context"
        const val PROP_FULL_SCREEN = "android-full-screen"
        const val PROP_ANDROID_HIDE_NAVIGATION_BAR = "android-hide-navigation-bar";
        const val PROP_CONTAINER_POPUP_TAG = "android-container-popup-tag"
        const val PROP_ADAPT_EDGE_TO_EDGE = "android-adapt-edge-to-edge"
    }

    private val mOverlayView = LynxOverlayView(context, this)
    var screenHeight = 0
    var screenWidth = 0

    init {
        super.insertChild(mOverlayView, 0)
        val activity = ContextUtils.getActivity(context)
        if (activity != null) {
            val metrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(activity)
            screenHeight = metrics.heightPixels
            screenWidth = metrics.widthPixels
        }
    }

    constructor(context: LynxContext) : this(context, null)

    override fun createView(context: Context): AndroidView {
        return object : AndroidView(context){
            override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
                setMeasuredDimension(0, 0)
            }

            override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {}

            override fun onVisibilityChanged(changedView: View, visibility: Int) {
                if (visibility == View.GONE || visibility == View.INVISIBLE) {
                    onDetach()
                } else if (visibility == View.VISIBLE) {
                    onAttach()
                }
            }
        }
    }

    override fun updateLayout(
        left: Int,
        top: Int,
        width: Int,
        height: Int,
        paddingLeft: Int,
        paddingTop: Int,
        paddingRight: Int,
        paddingBottom: Int,
        marginLeft: Int,
        marginTop: Int,
        marginRight: Int,
        marginBottom: Int,
        borderLeftWidth: Int,
        borderTopWidth: Int,
        borderRightWidth: Int,
        borderBottomWidth: Int,
        bound: Rect?
    ) {
        super.updateLayout(0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, null)
        val metrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(mContext)
        mOverlayView.updateLayout(0, 0, metrics.widthPixels, metrics.heightPixels, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, null)
    }

    override fun updateLayoutInfo(layout: LynxBaseUI?) {
        super.updateLayoutInfo(layout)
        mOverlayView.updateLayoutInfo(layout)
    }

    override fun updateDrawingLayoutInfo(left: Int, top: Int, bounds: Rect?): Boolean {
        super.updateDrawingLayoutInfo(left, top, bounds)
        return mOverlayView.updateDrawingLayoutInfo(left, top, bounds)
    }

    override fun insertChild(child: LynxBaseUI?, index: Int) {
        mOverlayView.insertChild(child, index)
    }

    override fun insertView(child: LynxUI<*>?) {
        mOverlayView.insertView(child)
    }

    override fun insertDrawList(mark: LynxBaseUI?, child: LynxBaseUI?) {
        mOverlayView.insertDrawList(mark, child)
    }

    override fun removeChild(child: LynxBaseUI?) {
        mOverlayView.removeChild(child)
    }

    override fun removeView(child: LynxBaseUI?) {
        mOverlayView.removeView(child)
    }

    override fun updateExtraData(extraData: Any?) {
        mOverlayView.updateExtraData(extraData)
    }

    override fun isUserInteractionEnabled(): Boolean {
        return false
    }

    override fun setParent(parent: UIParent?) {
        super.setParent(parent)
        mOverlayView.parent = parent
    }

    override fun onDetach() {
        mOverlayView.onDetach()
    }

    override fun setEvents(events: MutableMap<String, EventsListener>?) {
        mOverlayView.events = events
    }

    override fun getTouchEventDispatcher(): TouchEventDispatcher? {
        return mOverlayView.touchEventDispatcher
    }

        override fun updatePropertiesInterval(props: StylesDiffMap?) {
        mOverlayView.updatePropertiesInterval(props)
    }

    override fun afterPropsUpdated(props: StylesDiffMap?) {
        mOverlayView.afterPropsUpdated(props)
    }

    override fun setBackgroundColor(backgroundColor: Int) {
        mOverlayView.backgroundColor = backgroundColor
    }

    override fun getTransitionUI(): LynxUI<*> {
        return mOverlayView
    }

    override fun onAnimationUpdated() {
        mOverlayView.onAnimationUpdated()
    }

    override fun getChildCount(): Int {
        return mOverlayView.childCount
    }

    override fun getChildAt(index: Int): LynxBaseUI {
        return mOverlayView.getChildAt(index)
    }

    override fun getChildren(): MutableList<LynxBaseUI> {
        return mOverlayView.getChildren()
    }

    override fun setSign(sign: Int, tagName: String?) {
        super.setSign(sign, tagName)
        mOverlayView.setSign(sign, tagName)
    }

    override fun isViewVisible(): Boolean {
        return mOverlayView.isViewVisible()
    }

    override fun isOverlay(): Boolean {
        return true
    }
}
