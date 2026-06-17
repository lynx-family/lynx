// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.overlay

import com.lynx.tasm.behavior.shadow.*
import com.lynx.tasm.utils.DisplayMetricsHolder

open class LynxUIOverlayShadowNode : ShadowNode(), CustomMeasureFunc {
  init {
    setCustomMeasureFunc(this)
  }

  override fun measure(param: MeasureParam?, context: MeasureContext?): MeasureResult {
    if (childCount > 0) {
      val firstChild = getChildAt(0)
      if (firstChild is NativeLayoutNodeRef) {
        val metrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(mContext)
        val childParam = MeasureParam().apply {
          mHeight = metrics.heightPixels.toFloat()
          mWidth = metrics.widthPixels.toFloat()
          mWidthMode = MeasureMode.EXACTLY
          mHeightMode = MeasureMode.EXACTLY
        }
        firstChild.measureNativeNode(context, childParam)
        return MeasureResult(0.0f, 0.0f)
      }
    }
    return MeasureResult(0.0f, 0.0f)
  }

  override fun align(param: AlignParam?, context: AlignContext?) {
    if (childCount > 0) {
      val firstChild = getChildAt(0)
      if (firstChild is NativeLayoutNodeRef) {
        firstChild.alignNativeNode(context, AlignParam())
      }
    }
  }
}
