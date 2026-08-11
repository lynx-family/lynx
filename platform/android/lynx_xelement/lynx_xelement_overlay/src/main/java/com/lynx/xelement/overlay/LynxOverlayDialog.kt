// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.app.Activity
import android.app.Dialog
import android.content.Context
import android.graphics.PointF
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.fragment.app.FragmentManager
import com.lynx.tasm.behavior.event.EventTarget
import com.lynx.tasm.behavior.ui.LynxUI
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
    private var skipNativeDispatchForCurrentGesture = false
    private var passThroughTarget: PassThroughTarget? = null
    private var lastPassThroughEvent: MotionEvent? = null

    var containerPopupTag: String? = null

    var level: Number = 1

    private val statusBarHeight by lazy {
        getStatusBarHeight(overlay.lynxContext)
    }

    interface TouchEventListener {
        fun dispatchTouchEvent(ev: MotionEvent): Boolean
    }

    private sealed class PassThroughTarget {
        data class Overlay(val dialog: LynxOverlayDialog) : PassThroughTarget()
        data class HostActivity(val activity: Activity, val decorView: View) : PassThroughTarget()
        data class HostDialog(
            val owner: DialogFragment,
            val dialog: Dialog,
            val decorView: View
        ) : PassThroughTarget()
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
        if (ev.actionMasked == MotionEvent.ACTION_DOWN) {
            cancelPassThroughGesture()
            skipNativeDispatchForCurrentGesture = false
        } else if (passThroughTarget != null) {
            if (touchEventListener?.dispatchTouchEvent(ev) == true) {
                cancelPassThroughGesture()
                innerDispatchTouchEvent(ev)
                return false
            }
            return dispatchPassThroughTouchEvent(ev)
        }
        try {
            if (touchEventListener?.dispatchTouchEvent(ev) == true) {
                innerDispatchTouchEvent(ev)
                return false
            }

            val consumedByLynx = innerDispatchTouchEvent(ev)
            if (!consumedByLynx && skipNativeDispatchForCurrentGesture) {
                return true
            }
            if (consumedByLynx) {
                if (ev.actionMasked == MotionEvent.ACTION_DOWN) {
                    skipNativeDispatchForCurrentGesture = shouldSkipNativeDispatchForIgnoreFocus(ev)
                }
                if (skipNativeDispatchForCurrentGesture) {
                    return true
                }
                // If consumed, call super.dispatchTouchEvent rather
                // than call LynxOverlayManager.dispatchTouchEvent
                return super.dispatchTouchEvent(ev);
            }
            return LynxOverlayManager.dispatchTouchEvent(ev, this)
        } finally {
            if (ev.actionMasked == MotionEvent.ACTION_UP || ev.actionMasked == MotionEvent.ACTION_CANCEL) {
                skipNativeDispatchForCurrentGesture = false
            }
        }
    }

    private fun shouldSkipNativeDispatchForIgnoreFocus(ev: MotionEvent): Boolean {
        val target = overlay.hitTest(ev.x - overlay.getTransLeft(), ev.y - overlay.getTransTop())
        return target.ignoreFocus() && !isNativeInputTarget(target)
    }

    private fun isNativeInputTarget(target: EventTarget): Boolean {
        var current: EventTarget? = target
        while (current != null && current !is LynxUI<*>) {
            current = current.parent()
        }
        return current is LynxUI<*> && current.view is EditText
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

    internal fun dispatchPassThroughTouchEvent(event: MotionEvent): Boolean {
        if (event.actionMasked == MotionEvent.ACTION_DOWN) {
            clearPassThroughTarget()
            for (dialog in LynxOverlayManager.getPresentedOverlaysBelow(this)) {
                val mappedEvent = obtainEventForTarget(event, dialog.window?.decorView) ?: continue
                val consumed = dialog.innerDispatchTouchEvent(mappedEvent)
                if (consumed) {
                    passThroughTarget = PassThroughTarget.Overlay(dialog)
                    rememberPassThroughEvent(event)
                    val handled = dialog.superDispatchTouchEvent(mappedEvent)
                    mappedEvent.recycle()
                    return handled
                }
                mappedEvent.recycle()
            }
            if (overlay.isFragmentScoped()) {
                val target = resolveFragmentScopeTarget() ?: return false
                passThroughTarget = target
                rememberPassThroughEvent(event)
                return finishPassThroughEvent(event, dispatchToTarget(target, event))
            }
            return dispatchTouchEventToBelowContainer(event)
        }

        val target = passThroughTarget
        if (target == null) {
            return if (overlay.isFragmentScoped()) {
                false
            } else {
                dispatchTouchEventToBelowContainer(event)
            }
        }
        if (!isTargetValid(target)) {
            cancelPassThroughGesture()
            return false
        }
        rememberPassThroughEvent(event)
        return finishPassThroughEvent(event, dispatchToTarget(target, event))
    }

    internal fun cancelPassThroughGesture() {
        val target = passThroughTarget
        val lastEvent = lastPassThroughEvent
        passThroughTarget = null
        lastPassThroughEvent = null
        if (target != null &&
            lastEvent != null &&
            lastEvent.actionMasked != MotionEvent.ACTION_UP &&
            lastEvent.actionMasked != MotionEvent.ACTION_CANCEL) {
            val cancelEvent = MotionEvent.obtain(lastEvent)
            cancelEvent.action = MotionEvent.ACTION_CANCEL
            dispatchToTarget(target, cancelEvent)
            cancelEvent.recycle()
        }
        lastEvent?.recycle()
        skipNativeDispatchForCurrentGesture = false
    }

    internal fun cancelPassThroughGestureTargeting(dialog: LynxOverlayDialog) {
        val target = passThroughTarget
        if (target is PassThroughTarget.Overlay && target.dialog === dialog) {
            cancelPassThroughGesture()
        }
    }

    internal fun isPresentationActive(): Boolean {
        return overlay.isPresentationActive()
    }

    internal fun isFragmentScoped(): Boolean {
        return overlay.isFragmentScoped()
    }

    internal fun acceptsManualTouchDispatch(): Boolean {
        return !overlay.isNativeEventPassEnabled()
    }

    internal fun isSameHost(other: LynxOverlayDialog): Boolean {
        val activity = overlay.getHostActivity()
        val otherActivity = other.overlay.getHostActivity()
        val windowToken = overlay.getHostWindowToken()
        val otherWindowToken = other.overlay.getHostWindowToken()
        return activity != null &&
            activity === otherActivity &&
            windowToken != null &&
            windowToken == otherWindowToken
    }

    private fun resolveFragmentScopeTarget(): PassThroughTarget? {
        if (!overlay.isFragmentScopeActive()) {
            return null
        }
        val owner = overlay.getFragmentOwner() ?: return null
        val hostWindowToken = overlay.getHostWindowToken() ?: return null
        val dialogFragment = nearestDialogFragment(owner)
        if (dialogFragment != null) {
            val dialog = dialogFragment.dialog ?: return null
            val decorView = dialog.window?.decorView ?: return null
            if (!dialog.isShowing || decorView.windowToken != hostWindowToken) {
                return null
            }
            return PassThroughTarget.HostDialog(dialogFragment, dialog, decorView)
        }
        val activity = owner.activity ?: return null
        val decorView = activity.window?.decorView ?: return null
        if (decorView.windowToken != hostWindowToken) {
            return null
        }
        return PassThroughTarget.HostActivity(activity, decorView)
    }

    private fun isTargetValid(target: PassThroughTarget): Boolean {
        return when (target) {
            is PassThroughTarget.Overlay ->
                target.dialog.acceptsManualTouchDispatch() &&
                    LynxOverlayManager.isPresentedOnSameHost(this, target.dialog)
            is PassThroughTarget.HostActivity ->
                overlay.isFragmentScopeActive() &&
                    overlay.getFragmentOwner()?.let { owner ->
                        nearestDialogFragment(owner) == null && owner.activity === target.activity
                    } == true &&
                    target.decorView.windowToken != null &&
                    target.decorView.windowToken == overlay.getHostWindowToken()
            is PassThroughTarget.HostDialog ->
                overlay.isFragmentScopeActive() &&
                    overlay.getFragmentOwner()?.let { owner ->
                        nearestDialogFragment(owner) === target.owner
                    } == true &&
                    target.owner.dialog === target.dialog &&
                    target.dialog.isShowing &&
                    target.decorView.windowToken != null &&
                    target.decorView.windowToken == overlay.getHostWindowToken()
        }
    }

    private fun nearestDialogFragment(owner: Fragment): DialogFragment? {
        var fragment: Fragment? = owner
        while (fragment != null) {
            if (fragment is DialogFragment) {
                return fragment
            }
            fragment = fragment.parentFragment
        }
        return null
    }

    private fun dispatchToTarget(target: PassThroughTarget, event: MotionEvent): Boolean {
        return when (target) {
            is PassThroughTarget.Overlay -> {
                val mappedEvent =
                    obtainEventForTarget(event, target.dialog.window?.decorView) ?: return false
                target.dialog.innerDispatchTouchEvent(mappedEvent)
                val handled = target.dialog.superDispatchTouchEvent(mappedEvent)
                mappedEvent.recycle()
                handled
            }
            is PassThroughTarget.HostActivity -> {
                val mappedEvent = obtainEventForTarget(event, target.decorView) ?: return false
                val handled = target.activity.dispatchTouchEvent(mappedEvent)
                mappedEvent.recycle()
                handled
            }
            is PassThroughTarget.HostDialog -> {
                val mappedEvent = obtainEventForTarget(event, target.decorView) ?: return false
                val handled = target.dialog.dispatchTouchEvent(mappedEvent)
                mappedEvent.recycle()
                handled
            }
        }
    }

    private fun obtainEventForTarget(event: MotionEvent, targetDecorView: View?): MotionEvent? {
        val sourceDecorView = window?.decorView ?: return null
        targetDecorView ?: return null
        val sourceLocation = IntArray(2)
        val targetLocation = IntArray(2)
        sourceDecorView.getLocationOnScreen(sourceLocation)
        targetDecorView.getLocationOnScreen(targetLocation)
        return MotionEvent.obtain(event).apply {
            offsetLocation(
                (sourceLocation[0] - targetLocation[0]).toFloat(),
                (sourceLocation[1] - targetLocation[1]).toFloat()
            )
        }
    }

    private fun rememberPassThroughEvent(event: MotionEvent) {
        lastPassThroughEvent?.recycle()
        lastPassThroughEvent = MotionEvent.obtain(event)
    }

    private fun finishPassThroughEvent(event: MotionEvent, handled: Boolean): Boolean {
        if (event.actionMasked == MotionEvent.ACTION_UP ||
            event.actionMasked == MotionEvent.ACTION_CANCEL) {
            clearPassThroughTarget()
        }
        return handled
    }

    private fun clearPassThroughTarget() {
        passThroughTarget = null
        lastPassThroughEvent?.recycle()
        lastPassThroughEvent = null
    }

    //when event-pass-through is true, the event will dispatch to below container
    fun dispatchTouchEventToBelowContainer(event: MotionEvent): Boolean {
        if (overlay.isFragmentScoped()) {
            return false
        }
        val activity = ContextUtils.getActivity(overlay.lynxContext)
        return if (!containerPopupTag.isNullOrEmpty()) {
            val point = PointF(event.rawX, event.rawY)
            val fragment = (activity as? FragmentActivity)?.let {
                findFragmentByTagRecursive(it.supportFragmentManager, containerPopupTag, point)
            } as? Fragment ?: return false
            val offsetX = 0f
            val offsetY = getBelowContainerHeightOffset().toFloat()
            event.offsetLocation(-offsetX, -offsetY)
            val handled = if (fragment is DialogFragment) {
                fragment.dialog?.dispatchTouchEvent(event) ?: false
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


    /**
     * Find the fragment that should receive the pass-through touch event.
     *
     * Adapted for single-Activity apps: the target container may be nested several levels
     * deep as a child fragment, so the whole fragment tree is walked instead of only the
     * top level. A fragment is returned only when its [Fragment.getTag] matches [tag] - a
     * tag match is required, not merely whether [point] falls inside the fragment - so that
     * unrelated sibling fragments under the finger cannot swallow the event.
     *
     * Fragments are visited top-most first (reversed order) and depth-first (children before
     * their parent), so the inner-most, visually upper container wins when regions overlap.
     */
    private fun findFragmentByTagRecursive(
        fragmentManager: FragmentManager,
        tag: String?,
        point: PointF
    ): Fragment? {
        if (tag.isNullOrEmpty()) {
            return null
        }
        for (fragment in fragmentManager.fragments.reversed()) {
            if (!fragment.isAdded) {
                continue
            }
            // Search nested child fragments first so the inner-most match is preferred.
            findFragmentByTagRecursive(fragment.childFragmentManager, tag, point)?.let {
                return it
            }
            if (fragment.tag == tag && isPointInsideFragment(fragment, point)) {
                return fragment
            }
        }
        return null
    }

    /**
     * Whether [point] (in screen coordinates) falls inside [fragment]'s view bounds.
     *
     * A [DialogFragment] renders in its own window, so the on-screen location of its content
     * view is unreliable; for it we rely on the tag match alone and let the dialog hit-test
     * the event itself once it is dispatched.
     */
    private fun isPointInsideFragment(fragment: Fragment, point: PointF): Boolean {
        if (fragment is DialogFragment) {
            return true
        }
        val view = fragment.view ?: return false
        val location = IntArray(2)
        view.getLocationOnScreen(location)
        val left = location[0].toFloat()
        val top = location[1].toFloat()
        return point.x >= left && point.x <= left + view.width &&
            point.y >= top && point.y <= top + view.height
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
        cancelPassThroughGesture()
        if (!isInvalidContext(context)) {
            super.dismiss()
        }
    }

    private fun isInvalidContext(context: Context) :Boolean {
      return checkContextErrorCode(context) < 0
    }

}
