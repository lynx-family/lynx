// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map.google

import android.content.Context
import com.lynx.xelement.map.LynxAbstractMapView
import com.lynx.xelement.map.LynxMapProvider

class GoogleMapProvider : LynxMapProvider {
    override fun createMapView(context: Context): LynxAbstractMapView<*> = GoogleMapView(context)

    override fun createMapMarkerView(context: Context): LynxAbstractMapView.LynxAbstractMapMarkerView<*> =
        GoogleMarkerView(context)
}
