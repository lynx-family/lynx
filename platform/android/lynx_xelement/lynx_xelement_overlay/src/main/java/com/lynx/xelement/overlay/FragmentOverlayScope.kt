// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.view.View
import android.view.ViewTreeObserver
import androidx.core.view.ViewCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.lifecycle.Lifecycle

internal class FragmentOverlayScope(
    private var proxyView: View,
    private val listener: Listener
) : View.OnAttachStateChangeListener {

    interface Listener {
        fun onFragmentScopeChanged()
        fun onFragmentViewDestroyed()
        fun onFragmentOwnerNotFound()
    }

    var owner: Fragment? = null
        private set

    var isActive = false
        private set

    private var started = false
    private var observedViewTreeObserver: ViewTreeObserver? = null
    private val observedFragmentManagers = mutableListOf<FragmentManager>()

    private val resolveOwnerRunnable = Runnable { resolveOwner() }

    private val globalLayoutListener = ViewTreeObserver.OnGlobalLayoutListener {
        updateActiveState()
    }

    private val backStackChangedListener = FragmentManager.OnBackStackChangedListener {
        updateActiveState()
    }

    private val fragmentLifecycleCallbacks = object : FragmentManager.FragmentLifecycleCallbacks() {
        override fun onFragmentResumed(fragmentManager: FragmentManager, fragment: Fragment) {
            updateActiveState()
        }

        override fun onFragmentPaused(fragmentManager: FragmentManager, fragment: Fragment) {
            updateActiveState()
        }

        override fun onFragmentViewDestroyed(
            fragmentManager: FragmentManager,
            fragment: Fragment
        ) {
            if (fragment === owner) {
                isActive = false
                listener.onFragmentViewDestroyed()
            } else {
                updateActiveState()
            }
        }
    }

    fun start() {
        if (started) {
            return
        }
        started = true
        proxyView.addOnAttachStateChangeListener(this)
        registerGlobalLayoutListener()
        scheduleOwnerResolution()
    }

    fun updateProxyView(view: View): Boolean {
        if (!ViewCompat.isAttachedToWindow(view) || findOwningFragment(view) == null) {
            return false
        }
        if (proxyView === view) {
            return true
        }
        val wasStarted = started
        if (wasStarted) {
            stop()
        }
        proxyView = view
        if (wasStarted) {
            start()
        }
        return true
    }

    fun stop() {
        if (!started && owner == null) {
            return
        }
        started = false
        proxyView.removeCallbacks(resolveOwnerRunnable)
        proxyView.removeOnAttachStateChangeListener(this)
        unregisterGlobalLayoutListener()
        clearOwner()
    }

    fun reevaluate() {
        if (!started) {
            return
        }
        resolveOwner()
    }

    override fun onViewAttachedToWindow(view: View) {
        registerGlobalLayoutListener()
        scheduleOwnerResolution()
    }

    override fun onViewDetachedFromWindow(view: View) {
        proxyView.removeCallbacks(resolveOwnerRunnable)
        unregisterGlobalLayoutListener()
        setActive(false)
    }

    private fun scheduleOwnerResolution() {
        if (!started || !ViewCompat.isAttachedToWindow(proxyView)) {
            return
        }
        proxyView.removeCallbacks(resolveOwnerRunnable)
        proxyView.post(resolveOwnerRunnable)
    }

    private fun resolveOwner() {
        if (!started || !ViewCompat.isAttachedToWindow(proxyView)) {
            updateActiveState()
            return
        }
        val fragment = findOwningFragment(proxyView)
        if (fragment == null || viewLifecycleState(fragment) == null) {
            clearOwner()
            listener.onFragmentOwnerNotFound()
            return
        }
        if (owner !== fragment) {
            bindOwner(fragment)
        }
        updateActiveState()
    }

    private fun clearOwner() {
        observedFragmentManagers.forEach { fragmentManager ->
            fragmentManager.removeOnBackStackChangedListener(backStackChangedListener)
            fragmentManager.unregisterFragmentLifecycleCallbacks(fragmentLifecycleCallbacks)
        }
        observedFragmentManagers.clear()
        owner = null
        isActive = false
    }

    private fun bindOwner(fragment: Fragment) {
        clearOwner()
        owner = fragment
        var current: Fragment? = fragment
        while (current != null) {
            val fragmentManager = current.parentFragmentManager
            fragmentManager.addOnBackStackChangedListener(backStackChangedListener)
            fragmentManager.registerFragmentLifecycleCallbacks(fragmentLifecycleCallbacks, false)
            observedFragmentManagers.add(fragmentManager)
            current = current.parentFragment
        }
    }

    private fun updateActiveState() {
        val fragment = owner
        val nextActive = fragment != null &&
            ViewCompat.isAttachedToWindow(proxyView) &&
            proxyView.isShown &&
            isForegroundFragment(fragment)
        setActive(nextActive)
    }

    private fun isForegroundFragment(fragment: Fragment): Boolean {
        var current: Fragment? = fragment
        while (current != null) {
            if (!isVisibleAndResumed(current)) {
                return false
            }
            val fragmentManager = current.parentFragmentManager
            val primaryNavigationFragment = fragmentManager.primaryNavigationFragment
            if (primaryNavigationFragment != null) {
                if (primaryNavigationFragment !== current) {
                    return false
                }
            } else if (hasCompetingFragment(fragmentManager, current)) {
                return false
            }
            current = current.parentFragment
        }
        return true
    }

    private fun hasCompetingFragment(
        fragmentManager: FragmentManager,
        fragment: Fragment
    ): Boolean {
        val container = fragment.view?.parent ?: return true
        return fragmentManager.fragments.any { candidate ->
            candidate !== fragment &&
                candidate.view?.parent === container &&
                isVisibleAndResumed(candidate)
        }
    }

    private fun isVisibleAndResumed(fragment: Fragment): Boolean =
        fragment.isVisible &&
            viewLifecycleState(fragment)?.isAtLeast(Lifecycle.State.RESUMED) == true

    private fun viewLifecycleState(fragment: Fragment): Lifecycle.State? = try {
        fragment.viewLifecycleOwner.lifecycle.currentState
    } catch (error: IllegalStateException) {
        null
    }

    private fun setActive(nextActive: Boolean) {
        if (nextActive == isActive) {
            return
        }
        isActive = nextActive
        listener.onFragmentScopeChanged()
    }

    private fun registerGlobalLayoutListener() {
        if (!ViewCompat.isAttachedToWindow(proxyView)) {
            return
        }
        val viewTreeObserver = proxyView.viewTreeObserver
        if (!viewTreeObserver.isAlive || observedViewTreeObserver === viewTreeObserver) {
            return
        }
        unregisterGlobalLayoutListener()
        observedViewTreeObserver = viewTreeObserver
        viewTreeObserver.addOnGlobalLayoutListener(globalLayoutListener)
    }

    private fun unregisterGlobalLayoutListener() {
        val viewTreeObserver = observedViewTreeObserver ?: return
        if (viewTreeObserver.isAlive) {
            viewTreeObserver.removeOnGlobalLayoutListener(globalLayoutListener)
        }
        observedViewTreeObserver = null
    }

    private fun findOwningFragment(view: View): Fragment? = try {
        FragmentManager.findFragment(view)
    } catch (error: IllegalStateException) {
        null
    }
}
