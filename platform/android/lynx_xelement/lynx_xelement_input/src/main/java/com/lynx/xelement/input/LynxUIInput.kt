// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input

import android.content.Context
import android.os.Build
import android.text.TextUtils.TruncateAt
import android.view.Gravity
import android.view.inputmethod.EditorInfo
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.StylesDiffMap

open class LynxUIInput(context: LynxContext, params:Any?) : LynxUIBaseInput(context, params) {
  
  constructor(context: LynxContext) : this(context, null)
  
    override fun createView(context: Context?): LynxEditTextView{
        val editText = super.createView(context)
        editText.setLines(1)
        editText.isSingleLine = true
        editText.gravity = Gravity.CENTER_VERTICAL
        editText.setHorizontallyScrolling(true)
        editText.ellipsize = TruncateAt.END
        editText.setPadding(0, 0, 0, 0)
        editText.apply {
            setOnEditorActionListener { _, actionId, _ ->
                if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_GO ||
                        actionId == EditorInfo.IME_ACTION_SEARCH || actionId == EditorInfo.IME_ACTION_SEND
                        || actionId == EditorInfo.IME_ACTION_NEXT || actionId == EditorInfo.IME_NULL) {
                    onConfirm()
                    if (actionId != EditorInfo.IME_ACTION_NEXT) {
                        //when the action is next, don't need to hide keyboard
                        blur(null, null);
                        true
                    } else {
                        false
                    }
                } else {
                    false
                }
            }
        }
        return editText
    }

    override fun afterPropsUpdated(props: StylesDiffMap?) {
        super.afterPropsUpdated(props)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            mView.lineHeight = mFontSize.toInt()
        }
        mView.filters = arrayOf(readonlyInputFilter, maxLengthInputFilter, inputValueRegexFilter)
    }

    override fun triggerUpdateLayout(updatedHeight: Int) {
        val placeholderTextLayout = LynxInputUtils().getLayoutInEditText(mView.hint,
                mView,
                Int.MAX_VALUE,
                Int.MAX_VALUE)

        lynxContext.findShadowNodeBySign(sign)?.let {
            if (it is LynxUIBaseInputShadowNode) {
                it.updateHeightIfNeeded(placeholderTextLayout.height.coerceAtLeast(updatedHeight))
            }
        }
    }
}
