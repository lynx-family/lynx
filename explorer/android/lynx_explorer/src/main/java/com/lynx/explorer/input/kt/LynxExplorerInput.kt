// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.input.kt;

import android.content.Context
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import androidx.appcompat.widget.AppCompatEditText
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.ReadableMap
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.LynxUIMethodConstants
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.event.LynxCustomEvent

class LynxExplorerInput(context: LynxContext) : LynxUI<AppCompatEditText>(context) {

  override fun createView(context: Context): AppCompatEditText {
    return AppCompatEditText(context).apply {
      setLines(1)
      setSingleLine()
      gravity = Gravity.CENTER_VERTICAL
      background = null
      imeOptions = EditorInfo.IME_ACTION_NONE
      setHorizontallyScrolling(true)
      setPadding(0, 0, 0, 0)
      addTextChangedListener(object : TextWatcher {
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
        override fun afterTextChanged(s: Editable?) {
          emitEvent("input", mapOf("value" to (s?.toString() ?: "")))
        }
      })
      onFocusChangeListener = View.OnFocusChangeListener { _, hasFocus ->
        if (!hasFocus) {
          emitEvent("blur", null)
        }
      }
    }
  }

  override fun onLayoutUpdated() {
    super.onLayoutUpdated()
    val paddingTop = mPaddingTop + mBorderTopWidth
    val paddingBottom = mPaddingBottom + mBorderBottomWidth
    val paddingLeft = mPaddingLeft + mBorderLeftWidth
    val paddingRight = mPaddingRight + mBorderRightWidth
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom)
  }

  @LynxProp(name = "value")
  fun setValue(value: String) {
    if (value != mView.text.toString()) {
      mView.setText(value)
    }
  }

  @LynxUIMethod
  fun focus(params: ReadableMap, callback: Callback) {
    if (mView.requestFocus()) {
      if (showSoftInput()) {
        callback.invoke(LynxUIMethodConstants.SUCCESS)
      } else {
        callback.invoke(LynxUIMethodConstants.UNKNOWN, "fail to show keyboard")
      }
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "fail to focus")
    }
  }

  private fun showSoftInput(): Boolean {
    val imm = lynxContext.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
    return imm.showSoftInput(mView, InputMethodManager.SHOW_IMPLICIT, null)
  }

  @LynxProp(name = "placeholder")
  fun setPlaceHolder(value: String) {
    mView.hint = value
  }

  private fun emitEvent(name: String, value: Map<String, Any>?) {
    val detail = LynxCustomEvent(sign, name)
    value?.forEach { (key, v) -> detail.addDetail(key, v) }
    lynxContext.eventEmitter.sendCustomEvent(detail)
  }
}
