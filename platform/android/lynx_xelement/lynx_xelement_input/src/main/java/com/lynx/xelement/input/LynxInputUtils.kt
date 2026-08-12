// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.input

import android.graphics.Paint
import android.os.Build
import android.text.Layout
import android.text.StaticLayout
import android.text.TextDirectionHeuristic
import android.text.TextDirectionHeuristics
import android.text.TextUtils
import android.view.Gravity
import android.view.View
import android.widget.EditText
import com.lynx.tasm.behavior.shadow.text.StaticLayoutCompat
import com.lynx.tasm.behavior.shadow.text.TextAttributes
import java.lang.reflect.Method

class LynxInputUtils() {
    private data class LocaleLineHeightMethods(
        val isLocalePreferredLineHeightForMinimumUsed: Method,
        val getFontMetricsForLocale: Method,
        val setMinimumFontMetrics: Method
    )

    companion object {
        private const val API_LEVEL_35 = 35

        // Resolve these public API 35 methods lazily to keep compileSdkVersion at 33.
        private val localeLineHeightMethods by lazy {
            if (Build.VERSION.SDK_INT < API_LEVEL_35) {
                null
            } else {
                runCatching {
                    LocaleLineHeightMethods(
                        EditText::class.java.getMethod(
                            "isLocalePreferredLineHeightForMinimumUsed"
                        ),
                        Paint::class.java.getMethod(
                            "getFontMetricsForLocale",
                            Paint.FontMetrics::class.java
                        ),
                        StaticLayout.Builder::class.java.getMethod(
                            "setMinimumFontMetrics",
                            Paint.FontMetrics::class.java
                        )
                    )
                }.getOrNull()
            }
        }
    }

    private fun setLocalePreferredMinimumFontMetrics(
        builder: StaticLayout.Builder,
        editText: EditText
    ) {
        val methods = localeLineHeightMethods ?: return
        runCatching {
            if (methods.isLocalePreferredLineHeightForMinimumUsed.invoke(editText) != true) {
                return@runCatching
            }
            val minimumFontMetrics = Paint.FontMetrics()
            methods.getFontMetricsForLocale.invoke(editText.paint, minimumFontMetrics)
            methods.setMinimumFontMetrics.invoke(builder, minimumFontMetrics)
        }
    }

    fun getLayoutInEditText(charSequence: CharSequence,
                            editText: EditText,
                            measuredWidth: Int,
                            measuredHeight: Int): Layout {
        val textAlign = when (editText.gravity) {
            Gravity.LEFT -> Layout.Alignment.ALIGN_NORMAL
            Gravity.CENTER -> Layout.Alignment.ALIGN_CENTER
            Gravity.RIGHT -> Layout.Alignment.ALIGN_OPPOSITE
            else -> Layout.Alignment.ALIGN_NORMAL
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val textDirectionHeuristics = when (editText.textDirection) {
                View.TEXT_DIRECTION_LTR -> TextDirectionHeuristics.LTR
                View.TEXT_DIRECTION_RTL -> TextDirectionHeuristics.RTL
                View.TEXT_DIRECTION_LOCALE -> TextDirectionHeuristics.LOCALE
                else -> TextDirectionHeuristics.LTR
            }
            val builder = StaticLayout.Builder
                .obtain(
                    charSequence,
                    0,
                    charSequence.length,
                    editText.paint,
                    measuredWidth
                )
                .setAlignment(textAlign)
                .setTextDirection(textDirectionHeuristics)
                .setLineSpacing(editText.lineSpacingExtra, editText.lineSpacingMultiplier)
                .setIncludePad(editText.includeFontPadding)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                builder.setUseLineSpacingFromFallbacks(true)
            }
            if (Build.VERSION.SDK_INT >= API_LEVEL_35) {
                setLocalePreferredMinimumFontMetrics(builder, editText)
            }
            return builder.build()
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            val textDirectionHeuristics: TextDirectionHeuristic = when (editText.textDirection) {
                View.TEXT_DIRECTION_LTR -> TextDirectionHeuristics.LTR
                View.TEXT_DIRECTION_RTL -> TextDirectionHeuristics.RTL
                View.TEXT_DIRECTION_LOCALE -> TextDirectionHeuristics.LOCALE
                else -> TextDirectionHeuristics.LTR
            }
            return StaticLayoutCompat.get(
                charSequence,
                0,
                charSequence.length,
                editText.paint,
                measuredWidth,
                textAlign,
                editText.lineSpacingMultiplier,
                editText.lineSpacingExtra,
                editText.includeFontPadding,
                TextUtils.TruncateAt.END,
                TextAttributes.NOT_SET,
                textDirectionHeuristics
            )
        } else {
            return StaticLayout(
                charSequence,
                0,
                charSequence.length,
                editText.paint,
                measuredWidth,
                textAlign,
                editText.lineSpacingMultiplier,
                editText.lineSpacingExtra,
                editText.includeFontPadding,
                TextUtils.TruncateAt.END,
                measuredWidth
            )
        }
    }
}
