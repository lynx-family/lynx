// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.app.Activity
import android.app.Dialog
import android.content.Context
import android.os.Bundle
import android.view.MotionEvent
import android.view.ViewGroup
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import com.lynx.tasm.utils.ContextUtils

class LynxOverlayDialog(context: Context, private val overlay: LynxOverlayView): Dialog(context, R.style.Overlay) {

    companion object {
        //activity is finishing
        const val ERROR_CODE_IS_FINISHING = -1
        //activity is destroyed
        const val ERROR_CODE_IS_DESTROYED = -2
        //context is not activity
        const val ERROR_CODE_NOT_ACTIVITY_CONTEXT = 1
        //valid context
        const val ERROR_CODE_VALID = 0
    }

    private var touchEventListener: TouchEventListener? = null

    var containerPopupTag: String? = null

    var level: Number = 1

    private val statusBarHeight by lazy {
        getStatusBarHeight(overlay.lynxContext)
    }

    interface TouchEventListener {
        fun dispatchTouchEvent(ev: MotionEvent): Boolean
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window?.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        )
    }

    // Override dialog's dispatchTouchEvent, if dialog needs to consume,
    // call super.dispatchTouchEvent. Otherwise, call LynxOverlayManager.dispatchTouchEvent.
    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        if (touchEventListener?.dispatchTouchEvent(ev) == true) {
            innerDispatchTouchEvent(ev)
            return false
        }

        if(innerDispatchTouchEvent(ev)) {
            // If consumed, call super.dispatchTouchEvent rather
            // than call LynxOverlayManager.dispatchTouchEvent
            return super.dispatchTouchEvent(ev);
        }
        return LynxOverlayManager.dispatchTouchEvent(ev, this)
    }


    /**
     * This function is responsible for handling touch events on a view.
     *
     * @param ev a [MotionEvent] object that represents the touch event being handled.
     * @return a [Boolean] value that indicates whether the touch event was consumed or not.
     */
    fun innerDispatchTouchEvent(ev: MotionEvent): Boolean {
        var consumed = false
        if (handleTouchEvent(ev.x,ev.y)) {
            val offsetX = overlay.getTransLeft().toFloat()
            val offsetY = overlay.getTransTop().toFloat()
            ev.offsetLocation(-offsetX, -offsetY)
            consumed = overlay.getTouchEventDispatcher()?.onTouchEvent(ev, overlay) == true
            ev.offsetLocation(offsetX, offsetY)
        }
        return consumed
    }


    fun getSign():Int {
        return overlay.proxy.sign;
    }

    fun setTouchListener(listener: TouchEventListener?) {
        touchEventListener = listener
    }

    // LynxOverlayManager will call this function
    fun superDispatchTouchEvent(ev:MotionEvent):Boolean {
        return super.dispatchTouchEvent(ev);
    }

    //when event-pass-through is true, the event will dispatch to below container
    fun dispatchTouchEventToBelowContainer(event: MotionEvent): Boolean {
        val activity = ContextUtils.getActivity(overlay.lynxContext)
        return if (!containerPopupTag.isNullOrEmpty()) {
            val fragment = (activity as? FragmentActivity)?.let {
                findFragmentByTagRecursive(it.supportFragmentManager, containerPopupTag)
            } as? Fragment ?: return false
            val offsetX = 0f
            val offsetY = getBelowContainerHeightOffset().toFloat()
            event.offsetLocation(-offsetX, -offsetY)
            val handled = if (fragment is DialogFragment) {
                fragment.dialog.dispatchTouchEvent(event) ?: false
            } else {
                // For regular fragments, we need to convert screen coordinates to fragment view coordinates
                fragment.view?.let { fragmentView ->
                    // Get fragment view location on screen
                    val fragmentLocation = IntArray(2)
                    fragmentView.getLocationOnScreen(fragmentLocation)
                    val fragmentOffsetX = fragmentLocation[0].toFloat()
                    val fragmentOffsetY = fragmentLocation[1].toFloat()
                    
                    // Convert screen coordinates to fragment view coordinates
                    event.offsetLocation(-fragmentOffsetX, -fragmentOffsetY)
                    val result = fragmentView.dispatchTouchEvent(event)
                    // Restore original coordinates
                    event.offsetLocation(fragmentOffsetX, fragmentOffsetY)
                    result
                } ?: false
            }
            event.offsetLocation(offsetX, offsetY)
            handled
        } else {
            activity?.let {
                val offsetX = 0f
                val offsetY = getBelowContainerHeightOffset().toFloat()
                event.offsetLocation(-offsetX, -offsetY)
                val handled = it.dispatchTouchEvent(event)
                event.offsetLocation(offsetX, offsetY)
                return handled
            } ?: false
        }
    }

    //when content is not occupied in status bar, or the cut-out-mode is false, it need to minus the height of status bar
    private fun getBelowContainerHeightOffset() =
        if (!overlay.isStatusBarTranslucent() || !overlay.isCutOutMode()) {
            - statusBarHeight
        } else {
            0
        }

    private fun handleTouchEvent(x: Float, y: Float): Boolean {
        return overlay.needHandleEvent(x, y)
    }

    fun checkContextErrorCode(context: Context): Int {
        if (context is Activity) {
            if (context.isFinishing) {
                return ERROR_CODE_IS_FINISHING
            }
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1 &&
                context.isDestroyed) {
                return ERROR_CODE_IS_DESTROYED
            }
        } else {
            return ERROR_CODE_NOT_ACTIVITY_CONTEXT
        }
        return ERROR_CODE_VALID
    }


    private fun findFragmentByTagRecursive(fragmentManager: androidx.fragment.app.FragmentManager, tag: String?): androidx.fragment.app.Fragment? {
        val fragment = fragmentManager.findFragmentByTag(tag)
        if (fragment != null) {
            return fragment
        }
        for (childFragment in fragmentManager.fragments) {
            if (childFragment.isAdded) {
                val found = findFragmentByTagRecursive(childFragment.childFragmentManager, tag)
                if (found != null) {
                    return found
                }
            }
        }
        return null
    }

    private fun getStatusBarHeight(context: Context): Int {
        var height = 0
        val resourceId = context.resources.getIdentifier("status_bar_height", "dimen", "android")
        if (resourceId > 0) {
            height = context.resources.getDimensionPixelSize(resourceId)
        }
        return height
    }

    override fun dismiss() {
        if (!isInvalidContext(context)) {
            super.dismiss()
        }
    }

    private fun isInvalidContext(context: Context) :Boolean {
      return checkContextErrorCode(context) < 0
    }

}
