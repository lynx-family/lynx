// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import android.app.Dialog
import android.view.MotionEvent
import com.lynx.react.bridge.JavaOnlyArray
import com.lynx.devtoolwrapper.DevToolOverlayDelegate
import com.lynx.devtoolwrapper.OverlayService

//This manager will retain all overlay when is showing, and when it dismiss, it will removed by this manager.
object LynxOverlayManager {

    data class OverlayData(val id: String, val dialog: LynxOverlayDialog)

    private const val DEFAULT_OVERLAY_ID_PREFIX = "default_overlay_id_"
    //Within the GLOBAL_OVERLAYS list, elements with smaller indices have their dialogNG positioned above those with larger indices
    private val GLOBAL_OVERLAYS = mutableListOf<OverlayData>()
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
            GLOBAL_OVERLAYS.forEach {
                pushString(it.id)
            }
        }
    }

    // Returns arr where Dialogs at smaller indices correspond to dialogs positioned above those at larger indices.
    fun getGlobalOverlayView():ArrayList<Dialog>{
        val arr =  ArrayList<Dialog>();
        GLOBAL_OVERLAYS.forEach{
            arr.add(it.dialog);
        }
        return arr;
    }

    // Returns arr where signs at smaller indices correspond to Dialogs positioned above those at larger indices.
    fun getAllVisibleOverlaySign():ArrayList<Int>{
        var arr = ArrayList<Int>();
        GLOBAL_OVERLAYS.forEach{
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


    fun dispatchTouchEvent(ev: MotionEvent, overlay:LynxOverlayDialog): Boolean {
        GLOBAL_OVERLAYS.forEach {
            if (it.dialog.innerDispatchTouchEvent(ev) && overlay != it.dialog) {
                // if overlay != it.dialog and it.dialog need handleTouchEvent, dispatch event to it.dialog
                return it.dialog.superDispatchTouchEvent(ev)
            }
        }
        
        GLOBAL_OVERLAYS.takeIf {
            it.isNotEmpty()
        }?.let {
            return it[0].dialog.dispatchTouchEventToBelowContainer(ev)
        }

        return false
    }

}
