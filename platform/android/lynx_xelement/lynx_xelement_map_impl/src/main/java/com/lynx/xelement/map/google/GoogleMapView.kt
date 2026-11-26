// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map.google

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Point
import android.view.View
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.MapView
import com.google.android.gms.maps.model.BitmapDescriptorFactory
import com.google.android.gms.maps.model.CameraPosition
import com.google.android.gms.maps.model.LatLng
import com.google.android.gms.maps.model.LatLngBounds
import com.google.android.gms.maps.model.MapStyleOptions
import com.google.android.gms.maps.model.Marker
import com.google.android.gms.maps.model.MarkerOptions
import com.lynx.react.bridge.ReadableArray
import com.lynx.xelement.map.ILynxMapView
import com.lynx.xelement.map.LynxAbstractMapView
import com.lynx.xelement.map.LynxUIMap

internal class GoogleMapView(context: Context) : LynxAbstractMapView<Marker>(context) {
    private val nativeView = MapView(context)
    private var googleMap: GoogleMap? = null
    private var initialized = false
    private var resumed = false
    private var disposed = false
    private var receivingInitialProps = true
    private var pendingCamera: CameraPosition.Builder? = null
    private var cameraDuration = 0
    private val applyCamera = Runnable {
        val camera = pendingCamera ?: return@Runnable
        pendingCamera = null
        val map = googleMap ?: return@Runnable
        val movement = CameraUpdateFactory.newCameraPosition(camera.build())
        if (cameraDuration > 0) map.animateCamera(movement, cameraDuration, null)
        else map.moveCamera(movement)
    }
    private val pending = mutableListOf<(GoogleMap) -> Unit>()
    private val markers = mutableMapOf<String, Marker>()
    private val markerViews = mutableMapOf<String, ILynxMapView.ILynxMapMarkerView<Marker>>()

    override fun initializeMapView(params: HashMap<String, Any>, uiMap: LynxUIMap) {
        if (initialized || disposed) return
        initialized = true
        addView(nativeView, 0, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT))
        nativeView.onCreate(null)
        updateLifecycle()
        nativeView.getMapAsync { map ->
            if (disposed) return@getMapAsync
            googleMap = map
            map.setOnMapLoadedCallback { uiMap.sendEvent("mapload") }
            map.setOnMarkerClickListener { marker ->
                markerViews[marker.tag as? String]?.onClickMarker()
                true
            }
            for (action in pending) action(map)
            pending.clear()
            cameraDuration = 0
            applyCamera.run()
            receivingInitialProps = false
            // A marker can finish its Lynx layout while the map is starting.
            // Refresh the icon after attaching the queued markers to the map.
            for (markerView in markerViews.values) markerView.updateMarkerBitmap()
        }
    }

    private fun updateLifecycle() {
        if (!initialized || disposed) return
        val visible = isAttachedToWindow && windowVisibility == View.VISIBLE
        if (visible == resumed) return
        resumed = visible
        if (visible) {
            nativeView.onStart()
            nativeView.onResume()
        } else {
            nativeView.onPause()
            nativeView.onStop()
        }
    }

    override fun onWindowVisibilityChanged(visibility: Int) {
        super.onWindowVisibilityChanged(visibility)
        updateLifecycle()
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        updateLifecycle()
    }

    override fun onDetachedFromWindow() {
        if (resumed) {
            resumed = false
            nativeView.onPause()
            nativeView.onStop()
        }
        super.onDetachedFromWindow()
    }

    private fun withMap(action: (GoogleMap) -> Unit) {
        if (disposed) return
        val map = googleMap
        if (map == null) pending.add(action) else action(map)
    }

    override fun destroy() {
        if (disposed) return
        disposed = true
        removeCallbacks(applyCamera)
        pendingCamera = null
        pending.clear()
        markers.clear()
        markerViews.clear()
        googleMap?.setOnMapLoadedCallback(null)
        googleMap?.setOnMarkerClickListener(null)
        googleMap = null
        if (resumed) {
            resumed = false
            nativeView.onPause()
            nativeView.onStop()
        }
        if (initialized) nativeView.onDestroy()
    }

    override fun addMarker(id: String, metadata: HashMap<String, Any>,
                           marker: ILynxMapView.ILynxMapMarkerView<Marker>, firstRenderBitmap: Bitmap): Marker? {
        markerViews[id] = marker
        updateMarker(id, metadata, firstRenderBitmap)
        return markers[id]
    }

    override fun updateMarker(id: String, metadata: HashMap<String, Any>?, bitmap: Bitmap?) {
        val properties = metadata?.toMap()
        withMap { map ->
            if (id !in markerViews) return@withMap
            val coords = properties?.get("position") as? List<*>
            val lng = (coords?.getOrNull(0) as? Number)?.toDouble()
            val lat = (coords?.getOrNull(1) as? Number)?.toDouble()
            val position = if (lat != null && lng != null) LatLng(lat, lng) else null
            if (properties != null && position == null) {
                markers.remove(id)?.remove()
                return@withMap
            }
            val marker = markers[id] ?: position?.let {
                map.addMarker(MarkerOptions().position(it))?.apply {
                    tag = id
                    markers[id] = this
                }
            } ?: return@withMap
            if (position != null) marker.position = position
            if (properties != null) {
                val anchor = properties["anchor"] as? List<*>
                marker.setAnchor((anchor?.getOrNull(0) as? Number)?.toFloat() ?: 0.5f,
                                 (anchor?.getOrNull(1) as? Number)?.toFloat() ?: 0.5f)
                marker.alpha = (properties["alpha"] as? Number)?.toFloat() ?: 1f
                marker.rotation = (properties["rotateAngle"] as? Number)?.toFloat() ?: 0f
                marker.zIndex = (properties["zIndex"] as? Number)?.toFloat() ?: 1f
                marker.isDraggable = properties["draggable"] == true
            }
            if (bitmap != null) marker.setIcon(BitmapDescriptorFactory.fromBitmap(bitmap))
        }
    }

    internal fun marker(id: String): Marker? = markers[id]

    override fun removeMarker(id: String) {
        markerViews.remove(id)
        markers.remove(id)?.remove()
    }

    private fun setCamera(animated: Boolean, duration: Int, update: (CameraPosition.Builder) -> Unit) {
        withMap { map ->
            // Center and scale can arrive in the same Lynx update. The renderer still reports
            // the old position until the animation starts, so submit one camera movement.
            val camera = pendingCamera ?: CameraPosition.Builder(map.cameraPosition)
            update(camera)
            if (pendingCamera == null && !receivingInitialProps) post(applyCamera)
            pendingCamera = camera
            cameraDuration = if (animated) duration else 0
        }
    }

    override fun setCenter(latitude: Double, longitude: Double, animated: Boolean, duration: Int) =
        setCamera(animated, duration) { it.target(LatLng(latitude, longitude)) }

    private fun flushCamera() {
        if (pendingCamera == null) return
        removeCallbacks(applyCamera)
        applyCamera.run()
    }

    override fun getCenter(): Pair<Double, Double> {
        flushCamera()
        return googleMap?.cameraPosition?.target?.let { it.latitude to it.longitude } ?: (0.0 to 0.0)
    }

    override fun setScale(scale: Double, animated: Boolean, duration: Int) =
        setCamera(animated, duration) { it.zoom(scale.toFloat()) }

    override fun getScale(): Double {
        flushCamera()
        return googleMap?.cameraPosition?.zoom?.toDouble() ?: 0.0
    }

    override fun setRotate(rotate: Double, animated: Boolean, duration: Int) =
        setCamera(animated, duration) { it.bearing(rotate.toFloat()) }

    override fun setTilt(tilt: Double, animated: Boolean, duration: Int) =
        setCamera(animated, duration) { it.tilt(tilt.coerceIn(0.0, 90.0).toFloat()) }

    override fun setScaleRange(minScale: Double, maxScale: Double) = withMap {
        it.setMinZoomPreference(minScale.toFloat())
        it.setMaxZoomPreference(maxScale.toFloat())
    }

    override fun setBound(northeastLatitude: Double, northeastLongitude: Double, southwestLatitude: Double,
                          southwestLongitude: Double, animated: Boolean, duration: Int) =
        setBound(listOf(northeastLatitude to northeastLongitude, southwestLatitude to southwestLongitude),
                 emptyList(), animated, duration)

    override fun setBound(bound: List<Pair<Double, Double>>, padding: List<Int>, animated: Boolean, duration: Int) {
        if (bound.isEmpty()) return
        val bounds = LatLngBounds.builder()
        for (point in bound) bounds.include(LatLng(point.first, point.second))
        withMap { map ->
            if (width <= 0 || height <= 0) return@withMap
            flushCamera()
            val edge = padding.fold(0, ::maxOf).coerceIn(0, (minOf(width, height) - 1) / 2)
            val movement = CameraUpdateFactory.newLatLngBounds(bounds.build(), width, height, edge)
            if (animated && duration > 0) map.animateCamera(movement, duration, null) else map.moveCamera(movement)
        }
    }

    override fun getBound(): List<Pair<Double, Double>> {
        flushCamera()
        val bounds = googleMap?.projection?.visibleRegion?.latLngBounds ?: return emptyList()
        return listOf(bounds.northeast.latitude to bounds.northeast.longitude,
                      bounds.southwest.latitude to bounds.southwest.longitude)
    }

    override fun latLngToPx(latitude: Double, longitude: Double): Pair<Double, Double>? {
        flushCamera()
        val point = googleMap?.projection?.toScreenLocation(LatLng(latitude, longitude)) ?: return null
        val density = resources.displayMetrics.density.toDouble()
        return point.x / density to point.y / density
    }

    override fun pxToLatLng(x: Double, y: Double): Pair<Double, Double>? {
        flushCamera()
        val density = resources.displayMetrics.density
        val coords = googleMap?.projection?.fromScreenLocation(Point((x * density).toInt(), (y * density).toInt()))
        return coords?.let { it.latitude to it.longitude }
    }

    override fun setRotationDegree(rotationDegree: Double) = setRotate(rotationDegree, false, 0)
    override fun setEnableScaleControls(enable: Boolean) = withMap { it.uiSettings.isZoomControlsEnabled = enable }
    override fun setEnableScale(enable: Boolean) = withMap { it.uiSettings.isZoomGesturesEnabled = enable }
    override fun setEnableDrag(enable: Boolean) = withMap { it.uiSettings.isScrollGesturesEnabled = enable }
    override fun setEnableRotate(enable: Boolean) = withMap { it.uiSettings.isRotateGesturesEnabled = enable }
    override fun setEnableTilt(enable: Boolean) = withMap { it.uiSettings.isTiltGesturesEnabled = enable }
    override fun setMapStyle(mapStyle: String?) = withMap {
        it.setMapStyle(if (mapStyle?.startsWith("[") == true) MapStyleOptions(mapStyle) else null)
    }

    // The Google adapter does not expose these provider-specific controls or overlay formats.
    override fun setPointToCenter(x: Double, y: Double) {}
    override fun setLogoPosition(logoPosition: String) {}
    override fun setPolyline(polyline: ReadableArray?) {}
    override fun setRouteLabel(routeLabel: ReadableArray?) {}
    override fun setPolygons(polygons: ReadableArray?) {}
    override fun setCircles(circles: ReadableArray?) {}
    override fun setArcs(arcs: ReadableArray?) {}
}
