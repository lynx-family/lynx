// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map

import android.content.Context
import androidx.annotation.Keep
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.react.bridge.ReadableArray
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.LynxUIMethodConstants
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.event.LynxCustomEvent
import com.lynx.tasm.utils.PixelUtils
import kotlin.math.abs

@Keep
@LynxGeneratorName(packageName = "com.lynx.xelement.map")
@LynxBehavior(tagName = ["map"], isCreateAsync = false)
open class LynxUIMap(context: LynxContext, params: Any?): UIGroup<LynxAbstractMapView<*>>(context, params) {
    override fun createView(context: Context) = LynxXElementMapService.requireProvider().createMapView(context)

    override fun getMemoryUsageBytes(): Long = 100L * 1024 * 1024

    class LynxNodeReadyBlock(val runnable: Runnable, private val priority: Int = Int.MAX_VALUE) {
        val mRunnable:Runnable = runnable
        val mPriority:Int = priority
    }

    private var mLynxMapInitParams: HashMap<String, Any> = HashMap()
    private var mPriorityNodeReadyBlockArray: ArrayList<LynxNodeReadyBlock> = ArrayList()
    private var mCenterLatitude = 0.0
    private var mCenterLongitude = 0.0

    override fun onNodeReload() {
        super.onNodeReload()
        sendEvent("mapload")
    }

    override fun onNodeRemoved() {
        mView.destroy()
        super.onNodeRemoved()
    }

    override fun onInsertChild(child: LynxBaseUI, index: Int) {
        super.onInsertChild(child, index)
        if (child.tagName == "map-marker") {
            (child as LynxUIMapMarker).bindIMapView(mView)
        }
    }

    override fun onAnimationNodeReady() {
        super.onAnimationNodeReady()

        if (mIsFirstAnimatedReady) {
            mView.initializeMapView(mLynxMapInitParams, this)
        }
        mPriorityNodeReadyBlockArray.sortByDescending { it.mPriority }

        for (runnable in mPriorityNodeReadyBlockArray ) {
            runnable.mRunnable.run()
        }
        mPriorityNodeReadyBlockArray.clear()
    }

    override fun setNativeInteractionEnabled(enabled: Boolean) {
        mView.nativeInteractionEnabled = enabled
    }

    @LynxProp(name = "biz-name")
    open fun setBizName(bizName: String?) {
        mLynxMapInitParams["biz-name"] = bizName ?: "undefined"
    }

    @LynxProp(name = "map-type")
    open fun setMapType(mapType: Int?) {
        mLynxMapInitParams["map-type"] = mapType ?: -1
    }

    @LynxProp(name = "extra-params")
    open fun setExtraParams(extraParams: ReadableMap?) {
        if (extraParams != null) {
            val map = extraParams.toHashMap()
            if (map is Map<*, *>) {
                @Suppress("UNCHECKED_CAST")
                mLynxMapInitParams["extra-params"] = map as HashMap<String, Any>
            }
        } else {
            mLynxMapInitParams.remove("extra-params")
        }
    }

    @LynxProp(name = "map-style")
    open fun setMapStyle(mapStyle: String?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock({
            mView.setMapStyle(mapStyle)
        }))
    }

    @LynxProp(name = "center")
    open fun setCenter(latLng: ReadableMap?) {
        latLng?.let {
            val latitude = it.getDouble("latitude", 0.0)
            val longitude = it.getDouble("longitude", 0.0)
            if (abs(mCenterLatitude - latitude) >= 1e-6 || abs(mCenterLongitude - longitude) >= 1e-6) {
                mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock({
                    mView.setCenter(latitude, longitude)
                }))
            }
            mCenterLatitude = latitude
            mCenterLongitude = longitude
        }
    }

    @LynxProp(name = "scale")
    open fun setScale(scale: Double) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock({
            mView.setScale(scale)
        }, 100))
    }

    @LynxProp(name = "scale-range")
    open fun setScaleRange(scaleRange: ReadableArray?) {
        scaleRange?.let {
            if (it.size() >= 2) {
                val minScale = it.getDouble(0)
                val maxScale = it.getDouble(1)
                if (minScale <= maxScale) {
                    mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                            {
                                mView.setScaleRange(minScale, maxScale)
                            },
                    ))
                }
            }
        }
    }

    @LynxProp(name = "rotate")
    open fun setRotate(rotate: Double) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setRotationDegree(rotate)
                }
        ))
    }

    @LynxProp(name = "enable-scale-controls")
    open fun setEnableScaleControls(enable: Boolean) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setEnableScaleControls(enable)
                }
        ))
    }

    @LynxProp(name = "logo-position")
    open fun setLogoPosition(logoPosition: String) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setLogoPosition(logoPosition)
                }
        ))
    }

    @LynxProp(name = "enable-scale")
    open fun setEnableScale(enable: Boolean) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setEnableScale(enable)
                }
        ))
    }

    @LynxProp(name = "enable-drag")
    open fun setEnableDrag(enable: Boolean) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setEnableDrag(enable)
                }
        ))
    }

    @LynxProp(name = "enable-rotate")
    open fun setEnableRotate(enable: Boolean) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setEnableRotate(enable)
                }
        ))
    }

    @LynxProp(name = "enable-tilt")
    open fun setEnableTilt(enable: Boolean) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setEnableTilt(enable)
                }
        ))
    }

    @LynxProp(name = "polyline")
    open fun setPolyline(polylineArray: ReadableArray?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setPolyline(polylineArray)
                }
        ))
    }

    @LynxProp(name = "route-label")
    open fun setRouteLabel(routeLabelArray: ReadableArray?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
            {
                mView.setRouteLabel(routeLabelArray)
            }
        ))
    }

    @LynxProp(name = "polygons")
    open fun setPolygons(polygonArray: ReadableArray?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setPolygons(polygonArray)
                }
        ))
    }

    @LynxProp(name = "circles")
    open fun setCircles(circleArray: ReadableArray?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setCircles(circleArray)
                }
        ))
    }

    @LynxProp(name = "arcs")
    open fun setArcs(arcArray: ReadableArray?) {
        mPriorityNodeReadyBlockArray.add(LynxNodeReadyBlock(
                {
                    mView.setArcs(arcArray)
                }
        ))
    }

    @LynxUIMethod
    open fun setCenter(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val center = it.getMap("center", JavaOnlyMap())
            val latitude = center.getDouble("latitude", 0.0)
            val longitude = center.getDouble("longitude", 0.0)
            val animated = it.getBoolean("animated", true)
            val duration = it.getDouble("duration", 250.0).toInt()
            mView.setCenter(latitude, longitude, animated, duration)
            if (it.hasKey("scale")) {
                val scale = it.getDouble("scale")
                mView.setScale(scale, animated, duration)
            }
            callback?.invoke(LynxUIMethodConstants.SUCCESS)
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun setRotate(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val rotate = it.getDouble("rotate", 0.0)
            val animated = it.getBoolean("animated", true)
            val duration = it.getDouble("duration", 250.0).toInt()
            mView.setRotate(rotate, animated, duration)
            callback?.invoke(LynxUIMethodConstants.SUCCESS)
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun setTilt(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val tilt = it.getDouble("tilt", 0.0)
            val animated = it.getBoolean("animated", true)
            val duration = it.getDouble("duration", 250.0).toInt()
            mView.setTilt(tilt, animated, duration)
            callback?.invoke(LynxUIMethodConstants.SUCCESS)
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun getCenter(callback: Callback?) {
        val center = mView.getCenter()
        callback?.invoke(LynxUIMethodConstants.SUCCESS, JavaOnlyMap.from(mapOf(
                "latitude" to center.first,
                "longitude" to center.second
        )))
    }

    @LynxUIMethod
    open fun setScale(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val scale = it.getDouble("scale", 0.0)
            val animated = it.getBoolean("animated", true)
            val duration = it.getDouble("duration", 250.0).toInt()
            mView.setScale(scale, animated, duration)
            callback?.invoke(LynxUIMethodConstants.SUCCESS)
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun getScale(callback: Callback?) {
        callback?.invoke(LynxUIMethodConstants.SUCCESS, JavaOnlyMap.from(mapOf(
                "scale" to mView.getScale()
        )))
    }

    @LynxUIMethod
    open fun setBound(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val northeast = it.getMap("northeast", JavaOnlyMap())
            val southwest = params.getMap("southwest", JavaOnlyMap())
            val northeastLatitude = northeast.getDouble("latitude", 0.0)
            val northeastLongitude = northeast.getDouble("longitude", 0.0)
            val southwestLatitude = southwest.getDouble("latitude", 0.0)
            val southwestLongitude = southwest.getDouble("longitude", 0.0)
            val animated = it.getBoolean("animated", true)
            val duration = it.getDouble("duration", 250.0).toInt()
            mView.setBound(northeastLatitude, northeastLongitude, southwestLatitude, southwestLongitude, animated, duration)
            callback?.invoke(LynxUIMethodConstants.SUCCESS)
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun getBound(callback: Callback?) {
        val boundList = mView.getBound()
        if (boundList.size >= 2) {
            callback?.invoke(LynxUIMethodConstants.SUCCESS,  JavaOnlyMap.from(mapOf(
                    "northeast" to  JavaOnlyMap.from(mapOf(
                        "latitude" to boundList[0].first,
                        "longitude" to boundList[0].second)),
                    "southwest" to  JavaOnlyMap.from(mapOf(
                            "latitude" to boundList[1].first,
                            "longitude" to boundList[1].second))
            )))
        } else {
            callback?.invoke(LynxUIMethodConstants.SUCCESS, JavaOnlyMap.from(mapOf(
                    "northeast" to  JavaOnlyMap.from(mapOf(
                            "latitude" to 0,
                            "longitude" to 0)),
                    "southwest" to  JavaOnlyMap.from(mapOf(
                            "latitude" to 0,
                            "longitude" to 0))
            )))
        }
    }

    @LynxUIMethod
    open fun fitPoints(params: ReadableMap?, callback: Callback?) {
        params?.let {map ->
            val points = map.getArray("points")
            points?.let {
                val boundList = mutableListOf<Pair<Double, Double>>()
                var i = 0
                while (i < it.size()) {
                    val point = it.getMap(i++)
                    val latitude = point.getDouble("latitude", 0.0)
                    val longitude = point.getDouble("longitude", 0.0)
                    boundList.add(Pair(latitude, longitude))
                }

                val paddingList = mutableListOf<Int>()
                val paddingValue = map.getArray("padding")
                paddingValue?.let { value ->
                    var j = 0
                    while (j < value.size()) {
                        val dp = value.getDouble(j++)
                        paddingList.add(PixelUtils.dipToPx(dp).toInt())
                    }
                }

                val animated = map.getBoolean("animated", true)
                val duration = map.getDouble("duration", 250.0).toInt()
                mView.setBound(boundList, paddingList, animated, duration)
                callback?.invoke(LynxUIMethodConstants.SUCCESS)
            } ?: run {
                callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
            }
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun setAnchor(params: ReadableMap?, callback: Callback?) {
        params?.let {map ->
            val points = map.getMap("position")
            points?.let {
                val x = PixelUtils.dipToPx(it.getDouble("x", 0.0)).toDouble()
                val y = PixelUtils.dipToPx(it.getDouble("y", 0.0)).toDouble()
                mView.setPointToCenter(x, y)
                callback?.invoke(LynxUIMethodConstants.SUCCESS)
            } ?: run {
                callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
            }
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun latLngToPx(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val latLng = it.getMap("latLng")
            if (latLng == null) {
                callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, "Missing required parameter: latLng")
                return
            }
            val latitude = latLng.getDouble("latitude", 0.0)
            val longitude = latLng.getDouble("longitude", 0.0)
            val point = mView.latLngToPx(latitude, longitude)
            if (point != null) {
                callback?.invoke(LynxUIMethodConstants.SUCCESS, JavaOnlyMap.from(mapOf(
                    "x" to point.first,
                    "y" to point.second
                )))
            } else {
                callback?.invoke(LynxUIMethodConstants.INVALID_STATE_ERROR, "map is not loaded")
            }
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    @LynxUIMethod
    open fun pxToLatLng(params: ReadableMap?, callback: Callback?) {
        params?.let {
            val x = it.getDouble("x", 0.0)
            val y = it.getDouble("y", 0.0)
            val latLng = mView.pxToLatLng(x, y)
            if (latLng != null) {
                callback?.invoke(LynxUIMethodConstants.SUCCESS, JavaOnlyMap.from(mapOf(
                    "latLng" to JavaOnlyMap.from(mapOf(
                        "latitude" to latLng.first,
                        "longitude" to latLng.second
                    ))
                )))
            } else {
                callback?.invoke(LynxUIMethodConstants.INVALID_STATE_ERROR, "map is not loaded")
            }
        } ?: run {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID)
        }
    }

    override fun destroy() {
        super.destroy()
        mView.destroy()
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

}
