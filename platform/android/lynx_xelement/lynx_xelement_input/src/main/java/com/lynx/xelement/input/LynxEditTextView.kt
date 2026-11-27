// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Rect
import android.graphics.RectF
import android.os.SystemClock
import android.util.AttributeSet
import android.view.ActionMode
import android.view.InputDevice
import android.view.Menu
import android.view.MenuItem
import android.view.MotionEvent
import com.lynx.tasm.utils.ContextUtils
import kotlin.math.abs
import kotlin.math.ceil
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt

const val SCROLL_THRESHOLD = 10

class LynxEditTextView: LynxEditText {

    interface OnSelectionChangeListener {
        fun onSelectionChange(selStart: Int, selEnd: Int)
    }

    var onSelectionChangeListener:OnSelectionChangeListener? = null
    var onTextEditStateChanged: ((Int, Int, Int) -> Unit)? = null
    var textEditVertical: Boolean = true
    val textEditEnabled: Boolean = ContextUtils.toLynxContext(context)?.enableRawInput == true
    private var textEditCommandId = 0
    private var selectionAnchorStart = -1
    private var selectionAnchorEnd = -1
    private var collapseSelectionToStart = false
    private var textEditStatePending = false
    var textEditGeometryRevision = 0
        private set
    private var textEditLayoutWidth = 0
    private var textEditLayoutHeight = 0
    private val textEditStateUpdate = Runnable {
        textEditStatePending = false
        if (textEditEnabled && layout != null && width > 0 && height > 0) {
            onTextEditStateChanged?.invoke(
                textEditCommandId,
                if (textEditVertical) scrollY else scrollX,
                textEditMaximumScroll()
            )
        }
    }

    private var mAdjustInputMode = LYNX_EDIT_TEXT_LIGHT_INPUT_MODE_UNDEFINED
    private var mTouchStartX: Float = 0f
    private var mTouchStartY: Float = 0f
    private var mStartScrollY: Int = 0
    private var mIsScrolled: Boolean = false

    init {
        if (textEditEnabled) {
            customSelectionActionModeCallback = object : ActionMode.Callback {
                override fun onCreateActionMode(mode: ActionMode, menu: Menu) = true
                override fun onPrepareActionMode(mode: ActionMode, menu: Menu) = false
                override fun onActionItemClicked(mode: ActionMode, item: MenuItem) = false

                override fun onDestroyActionMode(mode: ActionMode) {
                    if (!collapseSelectionToStart) return
                    val start = selectionStart
                    val end = selectionEnd
                    val editable = text
                    // Keep native handles ordered. Move the caret only after Android
                    // actually collapses the range to the end when closing the menu.
                    post {
                        if (isFocused && text === editable &&
                            selectionStart == end && selectionEnd == end) {
                            setSelection(start)
                        }
                    }
                }
            }
        }
    }

    constructor(context: Context): super(context) {
    }

    constructor(context: Context, attrs: AttributeSet): super(context, attrs) {
    }

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int):
            super(context, attrs, defStyleAttr) {
    }

    override fun onSelectionChanged(selStart: Int, selEnd: Int) {
        super.onSelectionChanged(selStart, selEnd)
        collapseSelectionToStart = false
        onSelectionChangeListener?.onSelectionChange(selStart, selEnd)
        textEditGeometryRevision++
        scheduleTextEditState()
    }

    override fun onFocusChanged(focused: Boolean, direction: Int, previouslyFocusedRect: Rect?) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect)
        textEditGeometryRevision++
        scheduleTextEditState()
    }

    override fun onWindowFocusChanged(hasWindowFocus: Boolean) {
        if (!hasWindowFocus && textEditEnabled) {
            clearFocus()
        }
        super.onWindowFocusChanged(hasWindowFocus)
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        super.onLayout(changed, left, top, right, bottom)
        if (textEditLayoutWidth != width || textEditLayoutHeight != height) {
            textEditLayoutWidth = width
            textEditLayoutHeight = height
            textEditGeometryRevision++
        }
        scheduleTextEditState()
    }

    override fun onScrollChanged(left: Int, top: Int, oldLeft: Int, oldTop: Int) {
        super.onScrollChanged(left, top, oldLeft, oldTop)
        scheduleTextEditState()
    }

    override fun onTextChanged(text: CharSequence?, start: Int, lengthBefore: Int, lengthAfter: Int) {
        super.onTextChanged(text, start, lengthBefore, lengthAfter)
        textEditGeometryRevision++
        scheduleTextEditState()
    }

    override fun onDetachedFromWindow() {
        removeCallbacks(textEditStateUpdate)
        textEditStatePending = false
        selectionAnchorStart = -1
        selectionAnchorEnd = -1
        super.onDetachedFromWindow()
    }

    private fun scheduleTextEditState() {
        if (!textEditEnabled || textEditStatePending) return
        textEditStatePending = true
        post(textEditStateUpdate)
    }

    private fun textEditMaximumScroll(): Int {
        val textLayout = layout ?: return 0
        return if (textEditVertical) {
            max(0, textLayout.height - (height - compoundPaddingTop - compoundPaddingBottom))
        } else {
            max(0, ceil(textLayout.getLineWidth(0).toDouble()).toInt() -
                (width - compoundPaddingLeft - compoundPaddingRight))
        }
    }

    fun textEditCursorRect(): RectF? {
        val textLayout = layout ?: return null
        if (!isFocused || selectionEnd < 0 || selectionStart != selectionEnd) return null
        val position = selectionEnd.coerceAtMost(textLayout.text.length)
        val line = textLayout.getLineForOffset(position)
        val x = totalPaddingLeft + textLayout.getPrimaryHorizontal(position) - scrollX
        val top = totalPaddingTop + textLayout.getLineTop(line) - scrollY
        val bottom = totalPaddingTop + textLayout.getLineBottom(line) - scrollY
        return RectF(x, top.toFloat(), x + max(1f, resources.displayMetrics.density), bottom.toFloat())
    }

    fun applyTextEditCommand(type: String, commandId: Int, x: Float, y: Float, offset: Float) {
        if (!textEditEnabled || !isEnabled || !isFocusable ||
            commandId <= textEditCommandId || layout == null) return
        textEditCommandId = commandId
        when (type) {
            "scroll" -> {
                val bounded = offset.roundToInt().coerceIn(0, textEditMaximumScroll())
                if (textEditVertical) scrollTo(scrollX, bounded)
                else scrollTo(bounded, scrollY)
            }
            "tap" -> {
                selectionAnchorStart = -1
                collapseSelectionToStart = false
                val position = getOffsetForPosition(x, y)
                if (isFocused) {
                    projectTextEditTap(x, y, false)
                }
                setSelection(position)
            }
            "select" -> {
                if (isFocused) {
                    collapseSelectionToStart = false
                    projectTextEditTap(x, y, true)
                    selectionAnchorStart = min(selectionStart, selectionEnd)
                    selectionAnchorEnd = max(selectionStart, selectionEnd)
                }
            }
            "extendSelection" -> {
                if (selectionAnchorStart >= 0 && isFocused) {
                    val position = getOffsetForPosition(x, y)
                    setSelection(min(position, selectionAnchorStart), max(position, selectionAnchorEnd))
                    collapseSelectionToStart = position < selectionAnchorStart
                    bringPointIntoView(position)
                }
            }
            "endSelection" -> selectionAnchorStart = -1
        }
        scheduleTextEditState()
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun projectTextEditTap(x: Float, y: Float, longPress: Boolean) {
        val time = SystemClock.uptimeMillis()
        val down = MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN, x, y, 0)
        val up = MotionEvent.obtain(time, time + 1, MotionEvent.ACTION_UP, x, y, 0)
        down.source = InputDevice.SOURCE_TOUCHSCREEN
        up.source = InputDevice.SOURCE_TOUCHSCREEN
        try {
            // Compose has already recognized this gesture. Only project the editor operation.
            super.onTouchEvent(down)
            cancelLongPress()
            if (longPress) performLongClick()
            super.onTouchEvent(up)
        } finally {
            down.recycle()
            up.recycle()
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (!isFocusable) {
            return false
        }

        if (event == null) {
            return super.onTouchEvent(null)
        }

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                parent.requestDisallowInterceptTouchEvent(true)
                mTouchStartX = event.x
                mTouchStartY = event.y
                mStartScrollY = scrollY
            }
            MotionEvent.ACTION_MOVE -> {
                if ((!canScrollVertically(1) && event.y < mTouchStartY)
                    || (!canScrollVertically(-1) && event.y > mTouchStartY)) {
                    parent.requestDisallowInterceptTouchEvent(false)
                }
            }
            MotionEvent.ACTION_UP -> {
                parent.requestDisallowInterceptTouchEvent(false)
                mTouchStartX = 0f
                mTouchStartY = 0f
                mIsScrolled = Math.abs(scrollY - mStartScrollY) > SCROLL_THRESHOLD
            }
            MotionEvent.ACTION_CANCEL -> {
                parent.requestDisallowInterceptTouchEvent(false)
                mTouchStartX = 0f
                mTouchStartY = 0f
                mIsScrolled = abs(scrollY - mStartScrollY) > SCROLL_THRESHOLD
            }
        }
        return super.onTouchEvent(event)
    }
}
