// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map

import android.content.Context
import androidx.annotation.Keep
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.ReadableArray
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.LynxUIMethodConstants
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.event.LynxCustomEvent
import com.lynx.tasm.utils.PixelUtils

@Keep
@LynxGeneratorName(packageName = "com.lynx.xelement.map")
@LynxBehavior(tagName = ["map-marker"], isCreateAsync = false)
open class LynxUIMapMarker(context: LynxContext): UIGroup<LynxAbstractMapView.LynxAbstractMapMarkerView<*>>(context) {
    override fun createView(context: Context) = LynxXElementMapService.requireProvider().createMapMarkerView(context)

    private var mMarkerBeingCreated: Boolean = false
    private var mShouldRefreshBitmap: Boolean = false
    private var mShouldRefreshMetadata: Boolean = false
    private var mMetadata: HashMap<String, Any>? = null

    override fun onNodeReady() {
        super.onNodeReady()
        mMetadata?.let {
            if (mShouldRefreshMetadata) {
                mShouldRefreshMetadata = false
                if (!mMarkerBeingCreated) {
                    measureChildren()
                    layoutChildren()
                }
                mView.updateMetadata(it)
            }
            if (mShouldRefreshBitmap && mMarkerBeingCreated) {
                mShouldRefreshBitmap = false
                refreshMarker(null)
            }
            mMarkerBeingCreated = true
        }
    }

    override fun onLayoutUpdated() {
        super.onLayoutUpdated()
        mShouldRefreshBitmap = true
    }

    override fun onNodeRemoved() {
        super.onNodeRemoved()
        mView.destroy()
    }

    override fun destroy() {
        super.destroy()
        mView.destroy()
    }

    @LynxProp(name = "metadata")
    open fun setMetadata(metadata: ReadableMap?) {
        metadata?.asHashMap()?.let {
            mMetadata = it
            mShouldRefreshMetadata = true
        }
    }

    @LynxProp(name = "position")
    open fun setPosition(position: ReadableMap?) {
        position?.let {
            val latitude = it.getDouble("latitude", 0.0)
            val longitude = it.getDouble("longitude", 0.0)
            this.getMetadata()["position"] = listOf(longitude, latitude)
        } ?: run {
            this.getMetadata().remove("position")
        }
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "enable-drag", defaultBoolean = false)
    open fun setEnableDrag(value: Boolean) {
        this.getMetadata()["draggable"] = value
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "enable-click", defaultBoolean = true)
    open fun setEnableClick(value: Boolean) {
        this.getMetadata()["clickable"] = value
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "container-position")
    open fun setContainerPosition(position: ReadableMap?) {
        position?.let {
            val xPx = PixelUtils.dipToPx(it.getDouble("x", 0.0)).toDouble()
            val yPx = PixelUtils.dipToPx(it.getDouble("y", 0.0)).toDouble()
            this.getMetadata()["containerPosition"] = listOf(xPx, yPx)
        } ?: run {
            this.getMetadata().remove("containerPosition")
        }
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "anchor")
    open fun setAnchor(anchor: ReadableArray?) {
        anchor?.let {
            if (it.size() >= 2) {
                this.getMetadata()["anchor"] = listOf(it.getDouble(0), it.getDouble(1))
            }
        }  ?: run {
            this.getMetadata().remove("anchor")
        }
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "z-index", defaultInt = 1)
    open fun setZIndex(zIndex: Int) {
        this.getMetadata()["zIndex"] = zIndex
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "alpha", defaultDouble = 1.0)
    open fun setAlpha(alpha: Double) {
        this.getMetadata()["alpha"] = alpha
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "animate")
    open fun setAnimate(animate: ReadableMap?) {
        if (animate == null) {
            this.getMetadata().remove("animate")
            mShouldRefreshMetadata = true
            return
        }

        val animateMetadata = HashMap<String, Any>()
        appendAnimationConfig(animate, "scale", animateMetadata)
        appendAnimationConfig(animate, "alpha", animateMetadata)

        if (animateMetadata.isEmpty()) {
            this.getMetadata().remove("animate")
        } else {
            this.getMetadata()["animate"] = animateMetadata
        }
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "enable-collide-with-poi", defaultBoolean = false)
    open fun setEnableCollideWithPoi(value: Boolean) {
        this.getMetadata()["isCollideWithPoi"] = value
        mShouldRefreshMetadata = true
    }

    @LynxProp(name = "rotation", defaultDouble = 0.0)
    open fun setRotation(rotation: Double) {
        if (rotation in 0.0..360.0) {
            this.getMetadata()["rotateAngle"] = rotation
            mShouldRefreshMetadata = true
        }
    }

    @LynxUIMethod
    open fun refreshMarker(callback: Callback?) {
        measureChildren()
        layoutChildren()
        mView.updateMarkerBitmap()
        callback?.invoke(LynxUIMethodConstants.SUCCESS)
    }

    open fun bindIMapView(mapView: ILynxMapView<*>) {
        mView.attach(mapView, this)
    }

    open fun sendEvent(name: String, value: Map<String, Any>? = null) {
        val detail = LynxCustomEvent(sign, name)
        if (value != null) {
            for ((key, value1) in value) {
                detail.addDetail(key, value1)
            }
        }
        lynxContext.eventEmitter.sendCustomEvent(detail)
    }

    private fun getMetadata(): HashMap<String, Any> {
        if (mMetadata == null ) {
            mMetadata = HashMap()
        }
        return mMetadata as HashMap<String, Any>
    }

    private fun appendAnimationConfig(source: ReadableMap, key: String, target: HashMap<String, Any>) {
        val config = source.getMap(key)
        if (config == null) {
            return
        }

        val animationConfig = HashMap<String, Any>()
        if (config.hasKey("from")) {
            animationConfig["from"] = config.getDouble("from", 0.0)
        }
        if (config.hasKey("to")) {
            animationConfig["to"] = config.getDouble("to", 0.0)
        }
        if (config.hasKey("duration")) {
            animationConfig["duration"] = config.getInt("duration", 0)
        }
        if (config.hasKey("repeatCount")) {
            animationConfig["repeatCount"] = config.getInt("repeatCount", 0)
        }
        if (animationConfig.isNotEmpty()) {
            target[key] = animationConfig
        }
    }
}
