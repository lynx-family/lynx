// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input
import android.content.Context
import android.text.Editable
import android.text.InputFilter
import android.text.Layout
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.view.Gravity
import android.view.inputmethod.EditorInfo
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.StylesDiffMap
import com.lynx.tasm.event.LynxDetailEvent

open class LynxUITextArea(context: LynxContext, params: Any?) : LynxUIBaseInput(context, params) {

    private var mPreHeight:Int = -1

    private var mMaxLinesReached:Boolean = false

    private var maxHeightInputFilter:InputFilter? = null

    private var confirmEnterFilter = InputFilter { source, start, end, dest, dstart, dend ->
        if (isConfirmEnter() && source.toString() == "\n") {
            "";
        } else {
            null;
        }
    }
  
    constructor(context: LynxContext) : this(context, null)

    override fun createView(context: Context?): LynxEditTextView {
        val editText = super.createView(context)
        editText.setHorizontallyScrolling(false)
        editText.isSingleLine = false
        editText.gravity = Gravity.TOP
        editText.setPadding(0,0,0,0)
        editText.setOnEditorActionListener { _, action, _ ->
            if (isConfirmEnter()) {
                // Send event and blur manually.
                onConfirm();
                blur(null, null);
            }
            false
        }
        return editText
    }

    @LynxProp(name = "maxlines", defaultInt = 0)
    fun setMaxLines(maxLines: Int) {
        mView.maxLines = maxLines
        maxHeightInputFilter = InputFilter { source, start, end, dest, dstart, dend -> maxLinesFilter(source,start,end,dest,dstart,dend, maxLines) }
    }

    override fun afterPropsUpdated(props: StylesDiffMap?) {
        super.afterPropsUpdated(props)
        maxHeightInputFilter?.let {
            mView.filters = arrayOf(readonlyInputFilter, confirmEnterFilter, maxLengthInputFilter, maxHeightInputFilter, inputValueRegexFilter)
        } ?:run{
            mView.filters = arrayOf(readonlyInputFilter, confirmEnterFilter, maxLengthInputFilter, inputValueRegexFilter)
        }
    }

    private fun isConfirmEnter(): Boolean {
        // Send confirm while the return button is clicked
        val actionId = mView.imeOptions and EditorInfo.IME_MASK_ACTION
        if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_GO ||
            actionId == EditorInfo.IME_ACTION_SEARCH || actionId == EditorInfo.IME_ACTION_SEND
            || actionId == EditorInfo.IME_ACTION_NEXT) {
            return true
        }
        return false
    }

    private fun maxLinesFilter(
    source: CharSequence, start: Int, end: Int, dest: Spanned,
    dstart: Int, dend: Int, maxLines: Int): CharSequence {
        if (maxLines != Int.MAX_VALUE) {
            val inputUtils = LynxInputUtils()
            var textLayout: Layout
            var l = start
            var r = end
            val sourceBuilder = SpannableStringBuilder(source).subSequence(start, end)
            while(l < r) {
                val mid = (l + r) / 2
                val destBuilder = SpannableStringBuilder(dest)
                destBuilder.replace(dstart, dend,  sourceBuilder.subSequence(0, mid + 1))
                textLayout = inputUtils.getLayoutInEditText(destBuilder,
                    mView,
                    width,
                    Int.MAX_VALUE)
                if (textLayout.lineCount <= maxLines) {
                    l = mid + 1
                } else {
                    r = mid
                }
            }

            mMaxLinesReached = r < end
            if (mMaxLinesReached) {
              lynxContext.eventEmitter.sendCustomEvent(
                LynxDetailEvent(
                  sign,
                  "line"
                ).apply {
                  addDetail("line", -1)
                })
            }
            return source.subSequence(0, r)
        }
        return source
    }

    override fun afterTextDidChanged(s: Editable?) {
        val textLayout = LynxInputUtils().getLayoutInEditText(mView.text.toString(),
            mView,
            width,
            Int.MAX_VALUE)

        if (textLayout.height != mPreHeight && !mMaxLinesReached) {
            triggerUpdateLayout(textLayout.height)
            mPreHeight = textLayout.height

            lynxContext.eventEmitter.sendCustomEvent(
                LynxDetailEvent(
                    sign,
                    "line"
                ).apply {
                    addDetail("line", textLayout.lineCount)
                })
        }
    }
  
    override fun isTextArea(): Boolean {
      return true;
    }
}
