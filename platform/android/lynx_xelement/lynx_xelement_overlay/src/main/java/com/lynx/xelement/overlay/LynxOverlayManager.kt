// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.app.Dialog
import android.view.MotionEvent
import com.lynx.react.bridge.JavaOnlyArray
import com.lynx.devtoolwrapper.DevToolOverlayDelegate
import com.lynx.devtoolwrapper.OverlayService
import com.lynx.tasm.base.LLog

//This manager will retain all overlay when is showing, and when it dismiss, it will removed by this manager.
object LynxOverlayManager {

    data class OverlayData(val id: String, val dialog: LynxOverlayDialog)

    private const val DEFAULT_OVERLAY_ID_PREFIX = "default_overlay_id_"
    private const val TAG = "overlay"
    //Within the GLOBAL_OVERLAYS list, elements with smaller indices have their dialogNG positioned above those with larger indices
    private val GLOBAL_OVERLAYS = mutableListOf<OverlayData>()
    private val ACTIVE_DIALOGS = mutableSetOf<LynxOverlayDialog>()
    private var sCurrentId = 0

    private fun generateDefaultId(): String {
        return StringBuilder(DEFAULT_OVERLAY_ID_PREFIX).append(
            sCurrentId++).toString()
    }

    private class LynxOverlayServiceImpl : OverlayService {
        override fun getGlobalOverlayView(): ArrayList<Dialog> {
            return LynxOverlayManager.getGlobalOverlayView();
        }

        override fun getAllVisibleOverlaySign(): ArrayList<Int> {
            return LynxOverlayManager.getAllVisibleOverlaySign();
        }
    }

    init {
        DevToolOverlayDelegate.getInstance().init(LynxOverlayServiceImpl());
    }

    fun wrapEventParams(): JavaOnlyArray {
        return JavaOnlyArray().apply {
            visibleOverlays().forEach {
                pushString(it.id)
            }
        }
    }

    // Returns arr where Dialogs at smaller indices correspond to dialogs positioned above those at larger indices.
    fun getGlobalOverlayView():ArrayList<Dialog>{
        val arr =  ArrayList<Dialog>();
        visibleOverlays().forEach{
            arr.add(it.dialog);
        }
        return arr;
    }

    // Returns arr where signs at smaller indices correspond to Dialogs positioned above those at larger indices.
    fun getAllVisibleOverlaySign():ArrayList<Int>{
        var arr = ArrayList<Int>();
        visibleOverlays().forEach{
            arr.add(it.dialog.getSign());
        }
        return arr;
    }

    fun addGlobalId(dialog: LynxOverlayDialog): String? {
        val newId = generateDefaultId()
        val newLevel = dialog.level.toInt()
        val iterator = GLOBAL_OVERLAYS.listIterator()
        while (iterator.hasNext()) {
            val existingOverlay = iterator.next()
            if (newLevel <= existingOverlay.dialog.level.toInt()) {
                iterator.previous()
                iterator.add(OverlayData(newId, dialog))
                return newId
            }
        }
        GLOBAL_OVERLAYS.add(OverlayData(newId, dialog))
        return newId
    }

    fun removeGlobalId(id: String?) {
        id?.let {
            GLOBAL_OVERLAYS.forEach {
                if (it.id == id) {
                    ACTIVE_DIALOGS.remove(it.dialog)
                    GLOBAL_OVERLAYS.remove(it)
                    return
                }
            }
        }
    }

    fun containsGlobalId(id: String?): Boolean {
        id?.let {overlayId ->
            val item =  GLOBAL_OVERLAYS.firstOrNull {overlayData ->
                overlayData.id == overlayId
            }
            return item != null
        }

        return false
    }

    internal fun setGlobalIdActive(id: String?, active: Boolean) {
        val overlay = GLOBAL_OVERLAYS.firstOrNull { it.id == id } ?: return
        if (ACTIVE_DIALOGS.contains(overlay.dialog) == active) {
            return
        }
        if (active) {
            ACTIVE_DIALOGS.add(overlay.dialog)
            warnIfFragmentOwnersOverlap(overlay.dialog)
            return
        }
        ACTIVE_DIALOGS.remove(overlay.dialog)
        overlay.dialog.cancelPassThroughGesture()
        GLOBAL_OVERLAYS.map { it.dialog }.forEach {
            it.cancelPassThroughGestureTargeting(overlay.dialog)
        }
    }

    fun dispatchTouchEvent(ev: MotionEvent, overlay:LynxOverlayDialog): Boolean {
        if (overlay.isFragmentScoped() ||
            GLOBAL_OVERLAYS.any {
                ACTIVE_DIALOGS.contains(it.dialog) && it.dialog.isFragmentScoped()
            }) {
            return overlay.dispatchPassThroughTouchEvent(ev)
        }
        val activeOverlays = visibleOverlays()
        activeOverlays.forEach {
            if (it.dialog.innerDispatchTouchEvent(ev) && overlay != it.dialog) {
                // if overlay != it.dialog and it.dialog need handleTouchEvent, dispatch event to it.dialog
                return it.dialog.superDispatchTouchEvent(ev)
            }
        }
        
        activeOverlays.takeIf {
            it.isNotEmpty()
        }?.let {
            return it[0].dialog.dispatchTouchEventToBelowContainer(ev)
        }

        return false
    }

    private fun warnIfFragmentOwnersOverlap(dialog: LynxOverlayDialog) {
        if (!dialog.isFragmentScoped()) {
            return
        }
        val hasConflictingOwner = ACTIVE_DIALOGS.any {
            it !== dialog &&
                it.isFragmentScoped() &&
                dialog.isSameHost(it) &&
                !dialog.hasSameFragmentOwner(it)
        }
        if (hasConflictingOwner) {
            LLog.w(
                TAG,
                "Multiple Fragment-scoped overlay owners are active in the same host window"
            )
        }
    }

    internal fun cancelGesturesTargeting(target: LynxOverlayDialog) {
        GLOBAL_OVERLAYS.map { it.dialog }.forEach {
            it.cancelPassThroughGestureTargeting(target)
        }
    }

    internal fun getPresentedOverlaysBelow(source: LynxOverlayDialog): List<LynxOverlayDialog> {
        val sourceIndex = GLOBAL_OVERLAYS.indexOfFirst { it.dialog === source }
        if (sourceIndex < 0) {
            return emptyList()
        }
        return GLOBAL_OVERLAYS
            .subList(sourceIndex + 1, GLOBAL_OVERLAYS.size)
            .filter {
                ACTIVE_DIALOGS.contains(it.dialog) &&
                    it.dialog.isPresentationActive() &&
                    it.dialog.acceptsManualTouchDispatch() &&
                    source.isSameHost(it.dialog)
            }
            .map { it.dialog }
    }

    internal fun isPresentedOnSameHost(
        source: LynxOverlayDialog,
        target: LynxOverlayDialog
    ): Boolean {
        val data = GLOBAL_OVERLAYS.firstOrNull { it.dialog === target } ?: return false
        return ACTIVE_DIALOGS.contains(data.dialog) &&
            target.isPresentationActive() &&
            source.isSameHost(target)
    }

    private fun visibleOverlays(): List<OverlayData> {
        return GLOBAL_OVERLAYS.filter { ACTIVE_DIALOGS.contains(it.dialog) }
    }

}
