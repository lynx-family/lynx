// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.content.MutableContextWrapper
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.FrameLayout
import androidx.lifecycle.Lifecycle
import androidx.test.annotation.UiThreadTest
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.rule.ActivityTestRule
import com.lynx.react.bridge.DynamicFromMap
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.tasm.EventEmitter
import com.lynx.tasm.behavior.LynxUIOwner
import com.lynx.tasm.behavior.ui.UIBody
import com.lynx.tasm.event.EventsListener
import com.lynx.tasm.event.LynxCustomEvent
import com.lynx.tasm.utils.DisplayMetricsHolder
import com.lynx.testing.base.TestingLynxContext
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentMatchers.any
import org.mockito.Mockito.*

@RunWith(AndroidJUnit4::class)
class LynxOverlayViewTest {
    @get:Rule
    val activityRule = ActivityTestRule(FragmentOverlayScopeTestActivity::class.java)

    private lateinit var mutableContext: MutableContextWrapper
    private lateinit var overlay: LynxOverlayView
    private val events = mutableListOf<LynxCustomEvent>()

    @Before
    @UiThreadTest
    fun setUp() {
        val activity = activityRule.activity
        DisplayMetricsHolder.updateOrInitDisplayMetrics(activity)
        mutableContext = MutableContextWrapper(activity.applicationContext)
        val context = TestingLynxContext(mutableContext, activity.resources.displayMetrics)
        val owner = mock(LynxUIOwner::class.java)
        `when`(owner.context).thenReturn(context)
        val body = mock(UIBody::class.java)
        `when`(body.bodyView).thenReturn(UIBody.UIBodyView(context))
        `when`(owner.rootUI).thenReturn(body)
        context.uiBody = body
        context.lynxUIOwner = owner
        val emitter = mock(EventEmitter::class.java)
        doAnswer {
            val event = it.getArgument<LynxCustomEvent>(0)
            events.add(event)
            null
        }.`when`(emitter).sendCustomEvent(any(LynxCustomEvent::class.java))
        context.eventEmitter = emitter
        overlay = field(LynxUIOverlay(context), "mOverlayView")
        overlay.setLazyInitContext(true)
    }

    @After
    @UiThreadTest
    fun tearDown() {
        if (::overlay.isInitialized) overlay.destroy()
        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
    }

    @Test
    @UiThreadTest
    fun preloadUsesActivityWindowManagerAndPreservesProperties() {
        val original = dialog()
        overlay.setFullScreen(true)
        overlay.setAndroidSetSoftInputMode("pan")
        overlay.setContainerPopupTag("popup")
        overlay.setEvents(mutableMapOf("overlaytouch" to EventsListener("overlaytouch", "bind", "touch", null, null)))
        val touchListener: Any? = field(original, "touchEventListener")
        val flags = original.window!!.decorView.systemUiVisibility
        mutableContext.baseContext = activityRule.activity

        visible(true)

        assertEquals(listOf(0), showCodes())
        assertNotSame(original, dialog())
        assertTrue(dialog().isShowing)
        assertNotNull(dialog().window!!.attributes.token)
        val fullScreenFlags = View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        assertEquals(flags and fullScreenFlags, dialog().window!!.decorView.systemUiVisibility and fullScreenFlags)
        assertEquals(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN, dialog().window!!.attributes.softInputMode)
        assertEquals("popup", dialog().containerPopupTag)
        assertSame(touchListener, field<Any?>(dialog(), "touchEventListener"))
        assertEquals(listOf(dialog()), LynxOverlayManager.getGlobalOverlayView())
    }

    @Test
    @UiThreadTest
    fun omittedAdaptationDoesNotRebindPreloadedDialog() {
        val context = overlay.lynxContext
        overlay.destroy()
        overlay = field(LynxUIOverlay(context), "mOverlayView")
        val original = dialog()
        mutableContext.baseContext = activityRule.activity

        visible(true)

        assertSame(original, dialog())
        assertNull(original.window!!.decorView.windowToken)
        assertTrue(showCodes().isEmpty())
        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
    }

    @Test
    @UiThreadTest
    fun disabledAdaptationDoesNotRebindPreloadedDialog() {
        overlay.setLazyInitContext(false)
        val original = dialog()
        mutableContext.baseContext = activityRule.activity

        visible(true)

        assertSame(original, dialog())
        assertNull(original.window!!.decorView.windowToken)
        assertTrue(showCodes().isEmpty())
        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
    }

    @Test
    @UiThreadTest
    fun preloadPreservesSubPanelWindowType() {
        overlay.setLevel(3)
        overlay.setAdaptEdgeToEdge(true)
        mutableContext.baseContext = activityRule.activity

        visible(true)

        assertTrue(dialog().isShowing)
        assertEquals(WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL, dialog().window!!.attributes.type)
        assertEquals(3, dialog().level)
        assertEquals(listOf(0), showCodes())
    }

    @Test
    @UiThreadTest
    fun preloadedFragmentOverlayPreservesItsSessionAcrossSuspension() {
        val activity = activityRule.activity
        val container = FrameLayout(activity).apply { id = View.generateViewId() }
        activity.setContentView(container)
        val fragment = RetainedProxyFragment()
        activity.supportFragmentManager.beginTransaction()
            .add(container.id, fragment)
            .commitNow()
        mutableContext.baseContext = activity
        (fragment.requireView() as ViewGroup).addView(overlay.proxy.view)
        overlay.setAndroidOverlayScope("fragment")

        visible(true)
        val scope: FragmentOverlayScope = field(overlay, "mFragmentScope")
        scope.reevaluate()

        val presentedDialog = dialog()
        assertTrue(presentedDialog.isShowing)
        assertEquals(listOf(0), showCodes())
        activity.supportFragmentManager.beginTransaction()
            .setMaxLifecycle(fragment, Lifecycle.State.STARTED)
            .commitNow()
        scope.reevaluate()
        assertFalse(overlay.isPresentationActive())
        assertFalse(presentedDialog.isShowing)

        activity.supportFragmentManager.beginTransaction()
            .setMaxLifecycle(fragment, Lifecycle.State.RESUMED)
            .commitNow()
        scope.reevaluate()

        assertSame(presentedDialog, dialog())
        assertTrue(overlay.isPresentationActive())
        assertTrue(presentedDialog.isShowing)
        assertEquals(listOf(0), showCodes())
        assertEquals(listOf(presentedDialog), LynxOverlayManager.getGlobalOverlayView())
    }

    @Test
    @UiThreadTest
    fun activityBackedDialogIsReused() {
        mutableContext.baseContext = activityRule.activity
        val context = overlay.lynxContext
        overlay.destroy()
        overlay = field(LynxUIOverlay(context), "mOverlayView")
        overlay.setLazyInitContext(true)
        val original = dialog()

        visible(true)

        assertSame(original, dialog())
        assertTrue(original.isShowing)
        assertEquals(listOf(0), showCodes())
    }

    @Test
    @UiThreadTest
    fun missingActivityCanBeRetriedAfterContextUpdate() {
        visible(true)
        assertFalse(dialog().isShowing)
        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
        assertEquals(listOf(1), showCodes())

        mutableContext.baseContext = activityRule.activity
        visible(true)

        assertTrue(dialog().isShowing)
        assertEquals(listOf(1, 0), showCodes())
    }

    @Test
    @UiThreadTest
    fun repeatedShowAndHideDoNotDuplicateRegistrationsOrEvents() {
        mutableContext.baseContext = activityRule.activity
        visible(true)
        visible(true)
        assertEquals(listOf(0), showCodes())
        assertEquals(1, LynxOverlayManager.getGlobalOverlayView().size)

        visible(false)
        visible(false)

        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
        assertEquals(1, events.count { it.name == "dismissoverlay" })
        assertNull(field<Any?>(overlay, "mObserver"))
    }

    @Test
    @UiThreadTest
    fun destroyRemovesWindowAndObservers() {
        mutableContext.baseContext = activityRule.activity
        visible(true)
        assertTrue(dialog().isShowing)

        overlay.destroy()

        assertFalse(dialog().isShowing)
        assertTrue(field<List<*>>(LynxOverlayManager, "GLOBAL_OVERLAYS").isEmpty())
        assertNull(field<Any?>(overlay, "mObserver"))
    }

    private fun visible(value: Boolean) {
        overlay.setVisible(DynamicFromMap(JavaOnlyMap().apply { putBoolean("visible", value) }, "visible"))
        overlay.afterPropsUpdated(null)
    }

    private fun dialog(): LynxOverlayDialog = field(overlay, "mDialog")

    private fun showCodes(): List<Int> = events.filter { it.name == "showoverlay" }
        .map { it.eventParams()["errorCode"] as Int }

    @Suppress("UNCHECKED_CAST")
    private fun <T> field(target: Any, name: String): T = target.javaClass.getDeclaredField(name)
        .apply { isAccessible = true }.get(target) as T
}
