// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.os.Bundle
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.Lifecycle
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotSame
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

@RunWith(AndroidJUnit4::class)
class FragmentOverlayScopeTest {

    @Test
    fun ownerViewLifecycleRecreationRebindsRetainedProxy() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var fragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope
        lateinit var firstFragmentView: View

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                fragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, fragment)
                    .commitNow()
                firstFragmentView = fragment.requireView()
                scope = FragmentOverlayScope(fragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity {
                assertSame(fragment, scope.owner)
                assertTrue(scope.isActive)
                it.supportFragmentManager.beginTransaction()
                    .detach(fragment)
                    .commitNow()
                assertFalse(scope.isActive)
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity {
                it.supportFragmentManager.beginTransaction()
                    .attach(fragment)
                    .commitNow()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity {
                assertNotSame(firstFragmentView, fragment.requireView())
                assertSame(fragment, scope.owner)
                assertTrue(scope.isActive)
                assertEquals(1, listener.fragmentViewDestroyedCount)
                scope.stop()
            }
        }
    }

    @Test
    fun ownerNotFoundFailsClosed() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val proxyView = View(activity)
                activity.findViewById<FrameLayout>(FragmentOverlayScopeTestActivity.CONTAINER_ID)
                    .addView(proxyView)
                scope = FragmentOverlayScope(proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity {
                assertNull(scope.owner)
                assertFalse(scope.isActive)
                assertEquals(1, listener.ownerNotFoundCount)
                assertEquals(0, listener.fragmentViewDestroyedCount)
                scope.reevaluate()
                assertEquals(2, listener.ownerNotFoundCount)
                scope.stop()
            }
        }
    }

    @Test
    fun ownerIsActiveOnlyWhileResumed() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var fragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                fragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, fragment)
                    .commitNow()
                scope = FragmentOverlayScope(fragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                assertSame(fragment, scope.owner)
                assertTrue(scope.isActive)
                activity.supportFragmentManager.beginTransaction()
                    .setMaxLifecycle(fragment, Lifecycle.State.STARTED)
                    .commitNow()
                assertEquals(Lifecycle.State.STARTED, fragment.viewLifecycleOwner.lifecycle.currentState)
                assertSame(fragment, scope.owner)
                assertFalse(scope.isActive)
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                activity.supportFragmentManager.beginTransaction()
                    .setMaxLifecycle(fragment, Lifecycle.State.RESUMED)
                    .commitNow()
                assertEquals(Lifecycle.State.RESUMED, fragment.viewLifecycleOwner.lifecycle.currentState)
                assertSame(fragment, scope.owner)
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun unownedReplacementDoesNotDiscardFragmentOwner() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var fragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                fragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, fragment)
                    .commitNow()
                scope = FragmentOverlayScope(fragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                val unownedView = View(activity)
                activity.addContentView(unownedView, ViewGroup.LayoutParams(1, 1))
                assertFalse(scope.updateProxyView(unownedView))
                assertSame(fragment, scope.owner)
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun primaryNavigationFragmentControlsActiveState() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var firstFragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val fragmentManager = activity.supportFragmentManager
                firstFragment = RetainedProxyFragment()
                fragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, firstFragment)
                    .setPrimaryNavigationFragment(firstFragment)
                    .commitNow()
                scope = FragmentOverlayScope(firstFragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                val fragmentManager = activity.supportFragmentManager
                assertTrue(scope.isActive)

                val secondFragment = RetainedProxyFragment()
                fragmentManager.beginTransaction()
                    .add(FragmentOverlayScopeTestActivity.CONTAINER_ID, secondFragment)
                    .setPrimaryNavigationFragment(secondFragment)
                    .addToBackStack(null)
                    .commit()
                fragmentManager.executePendingTransactions()
                assertFalse(scope.isActive)

                fragmentManager.popBackStackImmediate()
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun multipleResumedFragmentsWithoutPrimaryNavigationFailClosed() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var firstFragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                firstFragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, firstFragment)
                    .commitNow()
                scope = FragmentOverlayScope(firstFragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                val fragmentManager = activity.supportFragmentManager
                assertNull(fragmentManager.primaryNavigationFragment)
                assertTrue(scope.isActive)

                val secondFragment = RetainedProxyFragment()
                fragmentManager.beginTransaction()
                    .add(FragmentOverlayScopeTestActivity.CONTAINER_ID, secondFragment)
                    .commitNow()
                assertFalse(scope.isActive)

                fragmentManager.beginTransaction()
                    .remove(secondFragment)
                    .commitNow()
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun ancestorPrimaryNavigationFragmentControlsNestedOwner() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var parentFragment: FragmentOverlayScopeParentFragment
        lateinit var ownerFragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                parentFragment = FragmentOverlayScopeParentFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, parentFragment)
                    .setPrimaryNavigationFragment(parentFragment)
                    .commitNow()
                ownerFragment = RetainedProxyFragment()
                parentFragment.childFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeParentFragment.CONTAINER_ID, ownerFragment)
                    .commitNow()
                scope = FragmentOverlayScope(ownerFragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                val fragmentManager = activity.supportFragmentManager
                assertTrue(scope.isActive)

                val nextParentFragment = FragmentOverlayScopeParentFragment()
                fragmentManager.beginTransaction()
                    .add(FragmentOverlayScopeTestActivity.CONTAINER_ID, nextParentFragment)
                    .setPrimaryNavigationFragment(nextParentFragment)
                    .addToBackStack(null)
                    .commit()
                fragmentManager.executePendingTransactions()
                assertFalse(scope.isActive)

                fragmentManager.popBackStackImmediate()
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun detachingProxyImmediatelyDeactivatesScope() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var fragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                fragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, fragment)
                    .commitNow()
                scope = FragmentOverlayScope(fragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity {
                assertTrue(scope.isActive)
                (fragment.proxyView.parent as ViewGroup).removeView(fragment.proxyView)
                assertFalse(scope.isActive)
                scope.stop()
            }
        }
    }

    @Test
    fun fragmentVisibilityControlsActiveStateWithoutLifecycleChange() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val listener = RecordingListener()
        lateinit var fragment: RetainedProxyFragment
        lateinit var scope: FragmentOverlayScope

        ActivityScenario.launch(FragmentOverlayScopeTestActivity::class.java).use { scenario ->
            scenario.onActivity { activity ->
                fragment = RetainedProxyFragment()
                activity.supportFragmentManager.beginTransaction()
                    .replace(FragmentOverlayScopeTestActivity.CONTAINER_ID, fragment)
                    .commitNow()
                scope = FragmentOverlayScope(fragment.proxyView, listener)
                scope.start()
            }
            instrumentation.waitForIdleSync()

            scenario.onActivity { activity ->
                assertTrue(scope.isActive)
                listener.expectScopeChange()
                activity.supportFragmentManager.beginTransaction()
                    .hide(fragment)
                    .commitNow()
                assertTrue(fragment.isHidden)
                assertEquals(Lifecycle.State.RESUMED, fragment.viewLifecycleOwner.lifecycle.currentState)
            }
            assertTrue(listener.awaitScopeChange())

            scenario.onActivity {
                assertFalse(scope.isActive)
                listener.expectScopeChange()
                it.supportFragmentManager.beginTransaction()
                    .show(fragment)
                    .commitNow()
                assertFalse(fragment.isHidden)
                assertEquals(Lifecycle.State.RESUMED, fragment.viewLifecycleOwner.lifecycle.currentState)
            }
            assertTrue(listener.awaitScopeChange())

            scenario.onActivity {
                assertTrue(scope.isActive)
                scope.stop()
            }
        }
    }

    private class RecordingListener : FragmentOverlayScope.Listener {
        var fragmentViewDestroyedCount = 0
        var ownerNotFoundCount = 0
        @Volatile
        private var scopeChangeLatch: CountDownLatch? = null

        fun expectScopeChange() {
            scopeChangeLatch = CountDownLatch(1)
        }

        fun awaitScopeChange(): Boolean {
            return scopeChangeLatch?.await(5, TimeUnit.SECONDS) == true
        }

        override fun onFragmentScopeChanged() {
            scopeChangeLatch?.countDown()
        }

        override fun onFragmentViewDestroyed() {
            fragmentViewDestroyedCount++
        }

        override fun onFragmentOwnerNotFound() {
            ownerNotFoundCount++
        }
    }
}

class FragmentOverlayScopeTestActivity : FragmentActivity() {

    val touchActions = mutableListOf<Int>()
    var recordTouches = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(FrameLayout(this).apply { id = CONTAINER_ID })
    }

    override fun dispatchTouchEvent(event: MotionEvent): Boolean {
        if (recordTouches) {
            touchActions.add(event.actionMasked)
            return true
        }
        return super.dispatchTouchEvent(event)
    }

    companion object {
        const val CONTAINER_ID = 0x4F560001
    }
}

class RetainedProxyFragment : Fragment() {
    lateinit var proxyView: View
        private set

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        if (!::proxyView.isInitialized) {
            proxyView = View(requireContext())
        }
        (proxyView.parent as? ViewGroup)?.removeView(proxyView)
        return FrameLayout(requireContext()).apply { addView(proxyView) }
    }

    override fun onDestroyView() {
        (proxyView.parent as? ViewGroup)?.removeView(proxyView)
        super.onDestroyView()
    }
}

class FragmentOverlayScopeParentFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = FrameLayout(requireContext()).apply { id = CONTAINER_ID }

    companion object {
        const val CONTAINER_ID = 0x4F560002
    }
}
