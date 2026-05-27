// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Build
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
          onConfirm();
          if (isConfirmEnter()) {
            // Send event and blur manually.
            blur(null, null);
          }
          mConfirmEnter
        }
        return editText
    }

    @LynxProp(name = "maxlines", defaultInt = Int.MAX_VALUE)
    fun setMaxLines(maxLines: Int) {
        val normalizedMaxLines = if (maxLines <= 0) Int.MAX_VALUE else maxLines
        mMaxLinesReached = false
        mView.maxLines = normalizedMaxLines
        maxHeightInputFilter = if (normalizedMaxLines == Int.MAX_VALUE) {
            null
        } else {
            InputFilter { source, start, end, dest, dstart, dend ->
                maxLinesFilter(source, start, end, dest, dstart, dend, normalizedMaxLines)
            }
        }
    }
  
    @LynxProp(name = "enable-scroll-bar", defaultBoolean = false )
    fun setEnableScrollBar(enable: Boolean){
      mView.isVerticalScrollBarEnabled = enable
      mView.isHorizontalScrollBarEnabled = false
      mView.isScrollbarFadingEnabled = true
      mView.scrollBarDefaultDelayBeforeFade = 1000
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        mView.verticalScrollbarThumbDrawable = ColorDrawable(Color.LTGRAY)
      }
    }

    override fun onNodeReady() {
      super.onNodeReady()
      val textLayout = LynxInputUtils().getLayoutInEditText(mView.text.toString(),
        mView,
        width,
        Int.MAX_VALUE)
  
      triggerUpdateLayout(textLayout.height)
    }

    override fun afterPropsUpdated(props: StylesDiffMap?) {
        super.afterPropsUpdated(props)
        maxHeightInputFilter?.let {
            mView.filters = arrayOf(confirmEnterFilter, inputValueRegexFilter, maxLengthInputFilter, maxHeightInputFilter, readonlyInputFilter)
        } ?:run{
            mView.filters = arrayOf(confirmEnterFilter, inputValueRegexFilter, maxLengthInputFilter, readonlyInputFilter)
        }
    }

    private fun isConfirmEnter(): Boolean {
        // Send confirm while the return button is clicked
        val actionId = mView.imeOptions and EditorInfo.IME_MASK_ACTION
        if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_GO ||
            actionId == EditorInfo.IME_ACTION_SEARCH || actionId == EditorInfo.IME_ACTION_SEND
            || actionId == EditorInfo.IME_ACTION_NEXT) {
            return !mConfirmEnter
        }
        return false
    }

    private fun maxLinesFilter(
    source: CharSequence, start: Int, end: Int, dest: Spanned,
    dstart: Int, dend: Int, maxLines: Int): CharSequence? {
        mMaxLinesReached = false
        if (maxLines == Int.MAX_VALUE || maxLines <= 0 || start >= end) {
            return null
        }

        val inputUtils = LynxInputUtils()
        val replacement = source.subSequence(start, end)
        var left = 0
        var right = replacement.length
        while (left < right) {
            val mid = (left + right + 1) / 2
            val destBuilder = SpannableStringBuilder(dest)
            destBuilder.replace(dstart, dend, replacement.subSequence(0, mid))
            val textLayout: Layout = inputUtils.getLayoutInEditText(destBuilder,
                mView,
                width,
                Int.MAX_VALUE)
            if (textLayout.lineCount <= maxLines) {
                left = mid
            } else {
                right = mid - 1
            }
        }

        mMaxLinesReached = left < replacement.length
        if (mMaxLinesReached) {
          lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(
              sign,
              "line"
            ).apply {
              addDetail("line", -1)
            })
          return replacement.subSequence(0, left)
        }
        return null
    }

    override fun afterTextDidChanged(s: Editable?) {
        val textLayout = LynxInputUtils().getLayoutInEditText(mView.text.toString(),
            mView,
            width,
            Int.MAX_VALUE)

        if (textLayout.height != mPreHeight) {
            triggerUpdateLayout(textLayout.height)
            mPreHeight = textLayout.height

            if (!mMaxLinesReached) {
              lynxContext.eventEmitter.sendCustomEvent(
                LynxDetailEvent(
                    sign,
                    "line"
                ).apply {
                    addDetail("line", textLayout.lineCount)
                })
            }
        }
        mMaxLinesReached = false
    }

  override fun triggerUpdateLayout(updatedHeight: Int) {
    val placeholderTextLayout = LynxInputUtils().getLayoutInEditText(mView.hint,
      mView,
      width,
      Int.MAX_VALUE)

    lynxContext.findShadowNodeAndRunTask(sign) { it ->
      if (it is LynxUIBaseInputShadowNode) {
        it.updateHeightIfNeeded(placeholderTextLayout.height.coerceAtLeast(updatedHeight))
      }
    }
  }
  
    override fun isTextArea(): Boolean {
      return true;
    }
}
