// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map

import android.graphics.Bitmap
import com.lynx.react.bridge.ReadableArray

interface ILynxMapView<M> {
    interface ILynxMapMarkerView<M> {

        fun attach(mapView: ILynxMapView<*>, markerUI: LynxUIMapMarker)
        fun updateMarkerBitmap()
        fun updateMetadata(metadata: HashMap<String, Any>)
        fun id(): String
        fun getMarkerObject(): M?
        fun onClickMarker()
        fun destroy()
    }

    fun initializeMapView(params: HashMap<String, Any>, uiMap: LynxUIMap)

    fun addMarker(id: String, metadata: HashMap<String, Any>, marker:ILynxMapMarkerView<M>, firstRenderBitmap: Bitmap): M?

    fun updateMarker(id: String, metadata: HashMap<String, Any>?,  bitmap: Bitmap?)

    fun removeMarker(id: String)

    fun destroy()

    fun setMapStyle(mapStyle: String?)

    fun setCenter(latitude: Double, longitude: Double, animated: Boolean = true, duration: Int = 250)

    fun getCenter(): Pair<Double, Double>

    fun setRotate(rotate: Double, animated: Boolean = true, duration: Int = 250)

    fun setTilt(tilt: Double, animated: Boolean = true, duration: Int = 250)

    fun setScale(scale: Double, animated: Boolean = true, duration: Int = 250)

    fun getScale(): Double

    fun setScaleRange(minScale: Double, maxScale: Double)

    fun setBound(northeastLatitude: Double, northeastLongitude: Double, southwestLatitude:Double, southwestLongitude:Double, animated: Boolean = true, duration: Int = 250)

    fun setBound(bound: List<Pair<Double, Double>>, padding: List<Int>, animated: Boolean = true, duration: Int = 250)

    fun getBound(): List<Pair<Double, Double>>

    fun setPointToCenter(x: Double, y: Double)

    fun setRotationDegree(rotationDegree: Double)

    fun setEnableScaleControls(enable: Boolean)

    fun setLogoPosition(logoPosition: String)

    fun setEnableScale(enable: Boolean)

    fun setEnableDrag(enable: Boolean)

    fun setEnableRotate(enable: Boolean)

    fun setEnableTilt(enable: Boolean)

    fun setPolyline(polyline: ReadableArray?)

    fun setRouteLabel(routeLabel: ReadableArray?)

    fun setPolygons(polygons: ReadableArray?)

    fun setCircles(circles: ReadableArray?)

    fun setArcs(arcs: ReadableArray?)

    fun latLngToPx(latitude: Double, longitude: Double): Pair<Double, Double>?

    fun pxToLatLng(x: Double, y: Double): Pair<Double, Double>?

}
