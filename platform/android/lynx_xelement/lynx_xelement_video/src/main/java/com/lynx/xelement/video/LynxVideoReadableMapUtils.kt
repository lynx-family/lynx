// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.video

import com.lynx.react.bridge.ReadableMap
import com.lynx.react.bridge.ReadableType

internal object LynxVideoReadableMapUtils {
    fun getNumberAsDouble(params: ReadableMap, name: String): Double =
        when (params.getType(name)) {
            ReadableType.Int -> params.getInt(name).toDouble()
            ReadableType.Long -> params.getLong(name).toDouble()
            ReadableType.Number -> params.getDouble(name, Double.NaN)
            else -> Double.NaN
        }
}
