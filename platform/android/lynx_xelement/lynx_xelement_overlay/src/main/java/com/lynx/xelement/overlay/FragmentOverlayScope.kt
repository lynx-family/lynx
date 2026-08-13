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
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.Observer

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

    private var lifecycleOwner: LifecycleOwner? = null
    private var generation = 0
    private var started = false
    private var resolutionScheduled = false
    private var resolutionAttemptedForAttachment = false
    private var viewLifecycleOwnerObserver: Observer<LifecycleOwner?>? = null
    private var observedViewTreeObserver: ViewTreeObserver? = null
    private var ownerFragmentManager: FragmentManager? = null

    private val globalLayoutListener = ViewTreeObserver.OnGlobalLayoutListener {
        updateActiveState()
    }

    private val backStackChangedListener = FragmentManager.OnBackStackChangedListener {
        updateActiveState()
    }

    private val lifecycleObserver = LifecycleEventObserver { _, event ->
        if (event == Lifecycle.Event.ON_DESTROY) {
            handleLifecycleOwnerDestroyed()
        } else {
            updateActiveState()
        }
    }

    fun start() {
        if (started) {
            return
        }
        started = true
        resolutionAttemptedForAttachment = false
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
        generation++
        resolutionScheduled = false
        resolutionAttemptedForAttachment = false
        proxyView.removeOnAttachStateChangeListener(this)
        unregisterGlobalLayoutListener()
        clearOwner()
    }

    fun reevaluate() {
        if (!started) {
            return
        }
        if (owner == null) {
            scheduleOwnerResolution()
        } else {
            updateActiveState()
        }
    }

    override fun onViewAttachedToWindow(view: View) {
        resolutionAttemptedForAttachment = false
        registerGlobalLayoutListener()
        scheduleOwnerResolution()
    }

    override fun onViewDetachedFromWindow(view: View) {
        generation++
        resolutionScheduled = false
        unregisterGlobalLayoutListener()
        setActive(false)
    }

    private fun scheduleOwnerResolution() {
        if (!started ||
            resolutionScheduled ||
            resolutionAttemptedForAttachment ||
            !ViewCompat.isAttachedToWindow(proxyView)) {
            updateActiveState()
            return
        }
        resolutionScheduled = true
        resolutionAttemptedForAttachment = true
        val scheduledGeneration = generation
        proxyView.post {
            if (!started || generation != scheduledGeneration) {
                return@post
            }
            resolutionScheduled = false
            resolveOwner()
        }
    }

    private fun resolveOwner() {
        val fragment = findOwningFragment(proxyView)
        val fragmentView = fragment?.view
        val resolvedLifecycleOwner = try {
            fragment?.viewLifecycleOwner
        } catch (error: IllegalStateException) {
            null
        }
        if (fragment == null ||
            fragmentView == null ||
            resolvedLifecycleOwner == null ||
            !contains(fragmentView, proxyView) ||
            fragmentView.windowToken == null ||
            fragmentView.windowToken != proxyView.windowToken) {
            clearOwner()
            listener.onFragmentOwnerNotFound()
            return
        }
        if (owner !== fragment || lifecycleOwner !== resolvedLifecycleOwner) {
            bindOwner(fragment, resolvedLifecycleOwner)
        }
        updateActiveState()
    }

    private fun clearOwner() {
        removeViewLifecycleOwnerObserver()
        unbindLifecycleOwner()
        ownerFragmentManager?.removeOnBackStackChangedListener(backStackChangedListener)
        ownerFragmentManager = null
        owner = null
        isActive = false
    }

    private fun bindOwner(fragment: Fragment, resolvedLifecycleOwner: LifecycleOwner) {
        if (owner !== fragment) {
            removeViewLifecycleOwnerObserver()
            unbindLifecycleOwner()
            ownerFragmentManager?.removeOnBackStackChangedListener(backStackChangedListener)
            owner = fragment
            ownerFragmentManager = fragment.parentFragmentManager
            ownerFragmentManager?.addOnBackStackChangedListener(backStackChangedListener)
            bindLifecycleOwner(resolvedLifecycleOwner)
            val observer = Observer<LifecycleOwner?> { newLifecycleOwner ->
                if (!started || owner !== fragment) {
                    return@Observer
                }
                if (newLifecycleOwner == null) {
                    handleLifecycleOwnerDestroyed()
                } else if (newLifecycleOwner !== lifecycleOwner) {
                    handleLifecycleOwnerDestroyed()
                    resolutionAttemptedForAttachment = false
                    scheduleOwnerResolution()
                } else {
                    updateActiveState()
                }
            }
            viewLifecycleOwnerObserver = observer
            fragment.viewLifecycleOwnerLiveData.observeForever(observer)
            return
        }
        bindLifecycleOwner(resolvedLifecycleOwner)
    }

    private fun bindLifecycleOwner(newLifecycleOwner: LifecycleOwner) {
        if (lifecycleOwner === newLifecycleOwner) {
            return
        }
        unbindLifecycleOwner()
        lifecycleOwner = newLifecycleOwner
        newLifecycleOwner.lifecycle.addObserver(lifecycleObserver)
    }

    private fun unbindLifecycleOwner() {
        lifecycleOwner?.lifecycle?.removeObserver(lifecycleObserver)
        lifecycleOwner = null
    }

    private fun removeViewLifecycleOwnerObserver() {
        val observer = viewLifecycleOwnerObserver ?: return
        owner?.viewLifecycleOwnerLiveData?.removeObserver(observer)
        viewLifecycleOwnerObserver = null
    }

    private fun handleLifecycleOwnerDestroyed() {
        if (lifecycleOwner == null) {
            return
        }
        unbindLifecycleOwner()
        generation++
        resolutionScheduled = false
        resolutionAttemptedForAttachment = false
        isActive = false
        listener.onFragmentViewDestroyed()
    }

    private fun updateActiveState() {
        val fragment = owner
        val primaryNavigationFragment = ownerFragmentManager?.primaryNavigationFragment
        val nextActive = fragment != null &&
            !resolutionScheduled &&
            fragment.isAdded &&
            !fragment.isHidden &&
            (primaryNavigationFragment == null || primaryNavigationFragment === fragment) &&
            lifecycleOwner?.lifecycle?.currentState?.isAtLeast(Lifecycle.State.RESUMED) == true &&
            ViewCompat.isAttachedToWindow(proxyView) &&
            proxyView.isShown
        setActive(nextActive)
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

    private fun contains(root: View, descendant: View): Boolean {
        var current: View? = descendant
        while (current != null) {
            if (current === root) {
                return true
            }
            current = current.parent as? View
        }
        return false
    }

    private fun findOwningFragment(view: View): Fragment? = try {
        FragmentManager.findFragment(view)
    } catch (error: IllegalStateException) {
        null
    }
}
