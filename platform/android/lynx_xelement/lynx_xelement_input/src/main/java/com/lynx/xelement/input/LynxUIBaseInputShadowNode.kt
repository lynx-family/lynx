// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input


import com.lynx.react.bridge.Dynamic
import com.lynx.react.bridge.ReadableType
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.PropsConstants
import com.lynx.tasm.behavior.shadow.LayoutNode
import com.lynx.tasm.behavior.shadow.MeasureMode
import com.lynx.tasm.behavior.shadow.MeasureOutput
import com.lynx.tasm.behavior.shadow.text.TextShadowNode
import com.lynx.tasm.utils.PixelUtils
import com.lynx.tasm.utils.UnitUtils
import kotlin.math.min

open class LynxUIBaseInputShadowNode : TextShadowNode() {
    private var mUIHeight:Int = 0
    override fun measure(node: LayoutNode, width: Float, widthMode: MeasureMode, height: Float, heightMode: MeasureMode): Long {
        var measuredHeight = height
        if (heightMode == MeasureMode.UNDEFINED) {
            measuredHeight = mUIHeight.toFloat()
        } else if (heightMode == MeasureMode.AT_MOST) {
            measuredHeight = min(mUIHeight.toFloat(), height);
        }
        return MeasureOutput.make(width, measuredHeight)
    }

    fun updateHeightIfNeeded(updatedHeight: Int) {
        if (updatedHeight != mUIHeight) {
            mUIHeight = updatedHeight
            this.resetIsDirty()
            this.markDirty()
            this.setNeedsLayoutForce()
        }
    }

    @LynxProp(name = PropsConstants.LINE_SPACING)
    fun setLineSpacingStr(lineSpacing: Dynamic) {
        var lineSpacingNumber = 0f
        if (lineSpacing.type == ReadableType.String) {
            lineSpacingNumber = UnitUtils.toPxWithDisplayMetrics(
                lineSpacing.asString(), 0f, 0f, 0f, 0f, -1.0f, context.screenMetrics
            )
        } else if (lineSpacing.type == ReadableType.Number || lineSpacing.type == ReadableType.Int) {
            lineSpacingNumber = PixelUtils.dipToPx(lineSpacing.asDouble())
        }
        // call super
        setLineSpacing(lineSpacingNumber)
    }
}
