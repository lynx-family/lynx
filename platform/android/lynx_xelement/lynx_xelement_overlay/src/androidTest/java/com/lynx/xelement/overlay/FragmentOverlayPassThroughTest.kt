// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.os.SystemClock
import android.util.DisplayMetrics
import android.view.MotionEvent
import android.view.ViewGroup
import androidx.fragment.app.FragmentActivity
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.lynx.react.bridge.DynamicFromArray
import com.lynx.react.bridge.JavaOnlyArray
import com.lynx.tasm.EventEmitter
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxUIOwner
import com.lynx.tasm.behavior.ui.UIBody
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView
import com.lynx.tasm.utils.DisplayMetricsHolder
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.Mockito

@RunWith(AndroidJUnit4::class)
class FragmentOverlayPassThroughTest {

    @Test
    fun hostGestureIsCancelledWhenInterceptedOrFragmentChanges() {
        var interceptMove = false

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val owner = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, owner)
                    .commitNow()
                val proxy = createOverlayProxy(activity)
                (owner.requireView() as ViewGroup).addView(proxy.view)
                val overlayView = getOverlayView(proxy)
                val dialog = getDialog(overlayView)
                dialog.setTouchListener(object : LynxOverlayDialog.TouchEventListener {
                    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
                        return interceptMove && ev.actionMasked == MotionEvent.ACTION_MOVE
                    }
                })
                overlayView.setEventsPassThrough(booleanDynamic(true))
                overlayView.setAndroidOverlayScope("fragment")
                overlayView.setVisible(booleanDynamic(true))
                overlayView.afterPropsUpdated(null)
                val fragmentScope = getFragmentScope(overlayView)
                fragmentScope.reevaluate()

                assertTrue(dialog.isShowing)
                activity.recordTouches = true
                val downTime = SystemClock.uptimeMillis()
                dispatch(dialog, downTime, downTime, MotionEvent.ACTION_DOWN)
                dispatch(dialog, downTime, downTime + 1, MotionEvent.ACTION_MOVE)
                assertEquals(
                    listOf(MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE),
                    activity.touchActions
                )

                interceptMove = true
                dispatch(dialog, downTime, downTime + 2, MotionEvent.ACTION_MOVE)
                assertEquals(
                    listOf(
                        MotionEvent.ACTION_DOWN,
                        MotionEvent.ACTION_MOVE,
                        MotionEvent.ACTION_CANCEL
                    ),
                    activity.touchActions
                )
                dispatch(dialog, downTime, downTime + 3, MotionEvent.ACTION_UP)

                interceptMove = false
                activity.touchActions.clear()
                val nextDownTime = SystemClock.uptimeMillis()
                dispatch(dialog, nextDownTime, nextDownTime, MotionEvent.ACTION_DOWN)
                val nextFragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .add(FragmentOverlayScopeTestActivity.CONTAINER_ID, nextFragment)
                    .setPrimaryNavigationFragment(nextFragment)
                    .commitNow()
                fragmentScope.reevaluate()
                assertEquals(
                    listOf(MotionEvent.ACTION_DOWN, MotionEvent.ACTION_CANCEL),
                    activity.touchActions
                )
                assertFalse(overlayView.isPresentationActive())
                overlayView.setVisible(booleanDynamic(false))
                overlayView.afterPropsUpdated(null)
            }
        }
    }

    private fun createOverlayProxy(context: FragmentActivity): LynxUIOverlay {
        DisplayMetricsHolder.updateOrInitDisplayMetrics(context, 3.0f)
        val displayMetrics = DisplayMetrics().apply {
            widthPixels = 1080
            heightPixels = 1920
            density = 3.0f
        }
        val lynxContext = object : LynxContext(context, displayMetrics) {
            override fun handleException(error: Exception) {}
        }
        val bodyView = UIBodyView(lynxContext)
        lynxContext.uiBody = UIBody(lynxContext, bodyView)
        lynxContext.setEventEmitter(Mockito.mock(EventEmitter::class.java))
        lynxContext.lynxUIOwner = LynxUIOwner(lynxContext, null, bodyView)
        return LynxUIOverlay(lynxContext)
    }

    private fun getOverlayView(proxy: LynxUIOverlay): LynxOverlayView {
        val field = LynxUIOverlay::class.java.getDeclaredField("mOverlayView")
        field.isAccessible = true
        return field.get(proxy) as LynxOverlayView
    }

    private fun getDialog(overlayView: LynxOverlayView): LynxOverlayDialog {
        val field = LynxOverlayView::class.java.getDeclaredField("mDialog")
        field.isAccessible = true
        return field.get(overlayView) as LynxOverlayDialog
    }

    private fun getFragmentScope(overlayView: LynxOverlayView): FragmentOverlayScope {
        val field = LynxOverlayView::class.java.getDeclaredField("mFragmentScope")
        field.isAccessible = true
        return field.get(overlayView) as FragmentOverlayScope
    }

    private fun booleanDynamic(value: Boolean): DynamicFromArray {
        val values = JavaOnlyArray()
        values.pushBoolean(value)
        return DynamicFromArray(values, 0)
    }

    private fun dispatch(
        dialog: LynxOverlayDialog,
        downTime: Long,
        eventTime: Long,
        action: Int
    ) {
        MotionEvent.obtain(downTime, eventTime, action, 50f, 50f, 0).let { event ->
            try {
                dialog.dispatchTouchEvent(event)
            } finally {
                event.recycle()
            }
        }
    }
}
