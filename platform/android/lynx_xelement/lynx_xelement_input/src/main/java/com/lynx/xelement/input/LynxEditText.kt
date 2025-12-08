// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input

import android.annotation.SuppressLint
import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import com.lynx.tasm.base.LLog

const val LYNX_EDIT_TEXT_LIGHT_INPUT_MODE_UNDEFINED = -1

open class LynxEditText: androidx.appcompat.widget.AppCompatEditText {

    companion object {
        private const val TAG = "LynxEditText"
    }

    var mPasting: Boolean = false;
    private var inputConnection: LynxInputConnectionWrapper? = null
    private var mCopyListener: CopyListener? = null
    private var isEditTextHasBeenServed: Boolean = false
    private var mAdjustInputMode = LYNX_EDIT_TEXT_LIGHT_INPUT_MODE_UNDEFINED
    var onAttachedToWindowListener: OnAttachedListener? = null

    interface CopyListener {
        fun onCopy(): Boolean
    }

    interface OnAttachedListener {
        fun onAttachedToWindow(inputMode: Int)
    }

    constructor(context: Context): super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet): super(context, attrs) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int):
            super(context, attrs, defStyleAttr) {
        init()
    }

    private fun init() {
        inputConnection = LynxInputConnectionWrapper(null, true)
        if (inputConnection != null) {
            inputConnection!!.bindEditText(this)
        } else {
            LLog.w(TAG, "InputConnection failed to initialize")
        }
    }

    fun inputConnection(): LynxInputConnectionWrapper? {
        if (!isEditTextHasBeenServed) {
            /* Once the input gets the focus once, the target of the inputConnection will not be null.
             * Even if it's not a target that can be used to pull up the keyboard.
             */
            LLog.w(TAG, "InputConnection has not been initialized yet ")
            return null
        }
        return inputConnection
    }

    // refer to comment, setOnKeyListener is only useful for hardware keyboards.
    // google input method(English) dosent trigger that.
    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection? {
        val ic = super.onCreateInputConnection(outAttrs) ?: return null
        try {
            inputConnection?.setTarget(ic)
        } catch (e: SecurityException) {
            return null
        }
        isEditTextHasBeenServed = true
        return inputConnection
    }

    fun setBackSpaceListener(backSpaceListener: LynxInputConnectionWrapper.BackspaceListener?) {
        inputConnection?.setBackspaceListener(backSpaceListener)
    }

    fun removeBackSpaceListener() {
        inputConnection?.removeBackspaceListener()
    }

    fun setCopyListener(copyListener: CopyListener) {
        mCopyListener = copyListener
    }

    fun removeCopyListener() {
        mCopyListener = null
    }

    override fun onTextContextMenuItem(id: Int): Boolean {
        when (id) {
            android.R.id.copy -> {
                return if (mCopyListener != null) {
                    mCopyListener!!.onCopy()
                } else {
                    super.onTextContextMenuItem(id)
                }
            }
            android.R.id.paste -> {
                mPasting = true;
                return super.onTextContextMenuItem(id)
            }       
            else -> return super.onTextContextMenuItem(id)
        }
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        isCursorVisible = true

        if (mAdjustInputMode != LYNX_EDIT_TEXT_LIGHT_INPUT_MODE_UNDEFINED) {
            onAttachedToWindowListener?.onAttachedToWindow(mAdjustInputMode)
        }
    }

    open fun updateInputMode(inputMode: Int) {
        mAdjustInputMode = inputMode
    }
}
