// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.util

import android.content.Context
import android.graphics.Color
import kotlin.math.abs


object Utils {
    fun dp2px(context: Context, dp: Float): Int {
        val scale = context.resources.displayMetrics.density
        return (dp * scale + 0.5f).toInt()
    }

    fun px2dip(context: Context, pxValue: Float): Int {
        val scale = context.resources.displayMetrics.density
        return (pxValue / scale + 0.5f).toInt()
    }

    fun rpx2px(context: Context, rpx: Float): Int {
        val widthPx = context.resources.displayMetrics.widthPixels;
        return (rpx * widthPx / 750).toInt();
    }

    fun px2rpx(context: Context, px: Int) : Int {
        val widthPx = context.resources.displayMetrics.widthPixels
        return (abs(px) * 750 / widthPx)
    }

    fun getWidthPixels(context: Context): Int {
        return context.resources.displayMetrics.widthPixels
    }

    fun String.toARGB(): Int {
        // 默认RGB 或者 RGBA
        return if (this.length <= 7) {
            Color.parseColor(this)
        } else {
            Color.parseColor(this).let {
                it ushr 8 or (it shl 24)
            }
        }
    }
}
