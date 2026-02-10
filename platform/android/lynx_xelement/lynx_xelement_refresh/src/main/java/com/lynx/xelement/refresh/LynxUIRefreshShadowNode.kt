// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.refresh

import android.text.TextUtils
import com.lynx.tasm.behavior.shadow.*

open class LynxUIRefreshShadowNode: ShadowNode(), CustomMeasureFunc {
  companion object {
    const val REFRESH_HEADER_TAG_NAME = "refresh-header"
  }
  
  override fun attachNativePtr(ptr: Long) {
    setCustomMeasureFunc(this)
    super.attachNativePtr(ptr)
  }

  override fun measure(param: MeasureParam?, context: MeasureContext?): MeasureResult {
    if (param == null) {
      return MeasureResult(0.0f, 0.0f)
    }
    for (i in 0 until this.childCount) {
      val childNode = getChildAt(i)
      if (childNode is NativeLayoutNodeRef) {
        val childMeasureParam = MeasureParam();
        if (TextUtils.equals(childNode.tagName, REFRESH_HEADER_TAG_NAME)) {
          childMeasureParam.updateConstraints(
              param.mWidth, param.mWidthMode, param.mHeight, MeasureMode.UNDEFINED)
        } else {
          childMeasureParam.updateConstraints(
              param.mWidth, param.mWidthMode, param.mHeight, MeasureMode.EXACTLY)
        }
        childNode.measureNativeNode(context, childMeasureParam)
      }
    }
    return MeasureResult(param.mWidth, param.mHeight)
  }

  override fun align(param: AlignParam?, context: AlignContext?) {
    for (i in 0 until this.childCount) {
      val node = getChildAt(i)
      if (node is NativeLayoutNodeRef) {
        val alignParam = AlignParam()
        alignParam.leftOffset = 0.0f
        alignParam.topOffset = 0.0f
        node.alignNativeNode(context, alignParam)
      }
    }
  }
}
