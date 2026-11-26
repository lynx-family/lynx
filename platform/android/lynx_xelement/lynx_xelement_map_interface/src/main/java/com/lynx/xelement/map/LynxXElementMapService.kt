// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.map

import android.content.Context

interface LynxMapProvider {
    fun createMapView(context: Context): LynxAbstractMapView<*>
    fun createMapMarkerView(context: Context): LynxAbstractMapView.LynxAbstractMapMarkerView<*>
}

object LynxXElementMapService {
    private val provider = java.util.concurrent.atomic.AtomicReference<LynxMapProvider>()

    /** Register the map implementation before creating any map elements. */
    @JvmStatic fun registerProvider(provider: LynxMapProvider) {
        this.provider.set(provider)
    }

    /** Supply a default when the host has not registered a map implementation. */
    @JvmStatic fun registerDefaultProvider(provider: LynxMapProvider) {
        this.provider.compareAndSet(null, provider)
    }

    internal fun requireProvider(): LynxMapProvider =
        checkNotNull(provider.get()) { "Register a map provider before creating map elements" }
}
