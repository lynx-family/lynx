// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map.google

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.view.MotionEvent
import com.google.android.gms.maps.model.Marker
import com.lynx.xelement.map.ILynxMapView
import com.lynx.xelement.map.LynxAbstractMapView
import com.lynx.xelement.map.LynxUIMapMarker

internal class GoogleMarkerView(context: Context) : LynxAbstractMapView.LynxAbstractMapMarkerView<Marker>(context) {
    private var map: GoogleMapView? = null
    private var ui: LynxUIMapMarker? = null
    private var metadata: HashMap<String, Any>? = null
    private var clickableMarker = true

    init {
        setWillNotDraw(false)
    }

    override fun attach(mapView: ILynxMapView<*>, markerUI: LynxUIMapMarker) {
        map = mapView as GoogleMapView
        ui = markerUI
        metadata?.let { updateMetadata(it) }
    }

    private fun snapshot(): Bitmap {
        val bitmap = Bitmap.createBitmap(maxOf(1, ui?.width ?: 0), maxOf(1, ui?.height ?: 0), Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        background?.let {
            it.setBounds(0, 0, bitmap.width, bitmap.height)
            it.draw(canvas)
        }
        super.dispatchDraw(canvas)
        return bitmap
    }

    // The map draws the bitmap at its geographic position. Keep the Lynx child
    // attached for layout and invalidations without painting it at the origin.
    override fun draw(canvas: Canvas) {
        if (getMarkerObject() != null) updateMarkerBitmap()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(
            MeasureSpec.makeMeasureSpec(maxOf(1, ui?.width ?: 0), MeasureSpec.EXACTLY),
            MeasureSpec.makeMeasureSpec(maxOf(1, ui?.height ?: 0), MeasureSpec.EXACTLY)
        )
    }

    override fun dispatchTouchEvent(event: MotionEvent?) = false

    override fun updateMetadata(metadata: HashMap<String, Any>) {
        this.metadata = metadata
        clickableMarker = metadata["clickable"] != false
        map?.addMarker(id(), metadata, this, snapshot())
    }

    override fun updateMarkerBitmap() {
        map?.updateMarker(id(), null, snapshot())
    }

    override fun id() = ui?.sign.toString()
    override fun getMarkerObject(): Marker? = map?.marker(id())
    override fun onClickMarker() {
        if (clickableMarker) ui?.sendEvent("clickmarker")
    }

    override fun destroy() {
        map?.removeMarker(id())
        metadata = null
        ui = null
        map = null
    }
}
