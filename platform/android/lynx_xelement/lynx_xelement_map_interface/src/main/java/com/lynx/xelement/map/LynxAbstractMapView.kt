// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map

import android.annotation.SuppressLint
import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.widget.FrameLayout
import com.lynx.tasm.behavior.ui.view.AndroidView

abstract class LynxAbstractMapView<M> @JvmOverloads constructor(
        context: Context,
        attrs: AttributeSet? = null,
        defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr), ILynxMapView<M> {

   abstract class LynxAbstractMapMarkerView<M> (
            context: Context,
    ) : AndroidView(context), ILynxMapView.ILynxMapMarkerView<M> {
    }

    var nativeInteractionEnabled = true
    var dragging = false

    init {
        initTouchListener()
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun initTouchListener() {
        setOnTouchListener { v, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    dragging = true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    dragging = false
                }
            }
            false
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        if (!this.nativeInteractionEnabled) {
            return true
        }
        if (hasMapGestures()) {
            when (ev.action) {
                MotionEvent.ACTION_DOWN -> {
                    parent?.requestDisallowInterceptTouchEvent(true)
                }

                MotionEvent.ACTION_MOVE -> {
                    parent?.requestDisallowInterceptTouchEvent(true)
                }

                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    parent?.requestDisallowInterceptTouchEvent(false)
                }
            }
        }
        return super.dispatchTouchEvent(ev)
    }

    open fun hasMapGestures(): Boolean {
        return false
    }
}
