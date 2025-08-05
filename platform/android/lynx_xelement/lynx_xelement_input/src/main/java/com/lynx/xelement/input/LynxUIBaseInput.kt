// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.input

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.os.Build
import android.text.Editable
import android.text.InputFilter
import android.text.InputType
import android.text.SpannableString
import android.text.Spanned
import android.text.TextWatcher
import android.text.method.PasswordTransformationMethod
import android.text.method.SingleLineTransformationMethod
import android.text.style.AbsoluteSizeSpan
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.Dynamic
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.react.bridge.ReadableMap
import com.lynx.react.bridge.ReadableType
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.LynxUIMethodConstants
import com.lynx.tasm.behavior.PropsConstants
import com.lynx.tasm.behavior.StyleConstants
import com.lynx.tasm.behavior.StylesDiffMap
import com.lynx.tasm.behavior.shadow.text.FontFamilySpan
import com.lynx.tasm.behavior.shadow.text.TypefaceCache
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.event.LynxDetailEvent
import com.lynx.tasm.fontface.FontFaceManager
import com.lynx.tasm.utils.ColorUtils
import com.lynx.tasm.utils.ContextUtils
import com.lynx.tasm.utils.PixelUtils
import com.lynx.tasm.utils.UnitUtils
import kotlin.math.max


open class LynxUIBaseInput(context: LynxContext, params: Any?) : LynxUI<LynxEditTextView>(context, params) {
  
  constructor(context:LynxContext) : this(context, null)
  
    companion object {
        const val UNDEFINED_INT: Int = Int.MIN_VALUE
        const val UNDEFINED_FLOAT: Float = Float.MIN_VALUE
        const val TYPE_NUMBER:String = "number"
        const val TYPE_TEXT:String = "text"
        const val TYPE_DIGIT:String = "digit"
        const val TYPE_PASSWORD:String = "password"
        const val TYPE_TEL:String = "tel"
        const val TYPE_EMAIL:String = "email"
    }
    private var mReadonly:Boolean = false;
    private var mFontWeight: Int = 400
    private var mPlaceholderFontWeight: Int = UNDEFINED_INT
    // TODO(xiamengfei.moonface): Impl line height with custom Span. private var mLineHeight: Float = -1f
    private var mLineSpacing: Float = -1f
    private var mFontStyle: Int = Typeface.NORMAL
    private var mPlaceholderFontStyle: Int = UNDEFINED_INT
    private var mFontFamilyName: String? = null
    private var mPlaceholderFontFamilyName: String? = null
    private var mPlaceholder: String? = null
    private var mPlaceholderFontSize: Float = UNDEFINED_FLOAT
    private var mPlaceholderFontColor: Int = ColorUtils.parse("#3c433c4c")
    private var mInputFilterRegex: String? = null
    var readonlyInputFilter = object: InputFilter {
      override fun filter(
        source: CharSequence,
        start: Int,
        end: Int,
        dest: Spanned?,
        dstart: Int,
        dend: Int
      ): CharSequence? {
        if (mReadonly) {
          return ""
        } else {
          return null
        }
      }
    }
    var maxLengthInputFilter = InputFilter.LengthFilter(Int.MAX_VALUE)
    var inputValueRegexFilter = object: InputFilter {
        override fun filter(
            source: CharSequence,
            start: Int,
            end: Int,
            dest: Spanned,
            dstart: Int,
            dend: Int
        ): CharSequence {
            mInputFilterRegex?.let {
                val builder = StringBuilder("")
                for (c: Char in source) {
                    if (c.toString().matches(it.toRegex())) {
                        builder.append(c)
                    }
                }
                return builder.toString();
            } ?: run {
                return source
            }
        }
    };

    private var mSoftInputModeStateStash =  WindowManager.LayoutParams.SOFT_INPUT_STATE_UNSPECIFIED
    private var mUseCustomKeyboard: Boolean = false

    open fun afterTextDidChanged(s: Editable?) {
    }

    override fun createView(context: Context?): LynxEditTextView {
        mFontSize = UnitUtils.toPxWithDisplayMetrics("14px", 0f, 0f, 0f, 0f, mContext?.screenMetrics)
        val editText = LynxEditTextView(context!!).apply {
            onFocusChangeListener =  View.OnFocusChangeListener { _: View?, hasFocus ->
                // Make sure keyboard is displayed while being focused
                if (hasFocus && !mUseCustomKeyboard) {
                    showSoftInput()
                }
                lynxContext.eventEmitter.sendCustomEvent(
                    LynxDetailEvent(
                        sign,
                        if (hasFocus) "focus" else "blur"
                    ).apply {
                        addDetail("value", text?.toString())
                    })
                if (hasFocus) {
                    var parentUI: LynxBaseUI? = parentBaseUI
                  lynxContext.EnsureEventDispatcher()
                    while (parentUI != null) {
                        if (parentUI.tagName == "x-overlay-ng") {
                            parentUI.touchEventDispatcher?.setFocusedUI(this@LynxUIBaseInput)
                            break
                        }
                        parentUI = parentUI.parentBaseUI
                    }

                    lynxContext.touchEventDispatcher?.setFocusedUI(this@LynxUIBaseInput)

                }
            }

            onAttachedToWindowListener = object : LynxEditText.OnAttachedListener {
                override fun onAttachedToWindow(inputMode: Int) {
                    var parent: LynxBaseUI? = parentBaseUI
                    while (parent != null) {
                        if (parent.tagName == "x-overlay-ng") {
                            parent.window?.setSoftInputMode(inputMode)
                            return
                        }
                        parent = parent.parentBaseUI
                    }
                    window?.setSoftInputMode(inputMode)
                }

            }

            background = null

            imeOptions = EditorInfo.IME_ACTION_NONE

            addTextChangedListener(object : TextWatcher {
                override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {
                }

                override fun afterTextChanged(s: Editable?) {
                    afterTextDidChanged(s)
                    if (mView != null) {
                      s?.let {
                        lynxContext.eventEmitter.sendCustomEvent(LynxDetailEvent(
                          sign,
                          "input"
                        ).apply {
                          addDetail("value", it.toString())
                          var selectionStart = -1
                          if (mView.isFocused) {
                            selectionStart = mView.selectionStart
                          }
                          var selectionEnd = -1
                          if (mView.isFocused) {
                            selectionEnd = mView.selectionEnd
                          }
                          addDetail("selectionStart", selectionStart)
                          addDetail("selectionEnd", selectionEnd)
                          addDetail("isComposing", mView?.inputConnection()?.hasComposingText(it))
                        })
                      }
                    }
                }

                    override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                    }
                })

            onSelectionChangeListener = object : LynxEditTextView.OnSelectionChangeListener {
                override fun onSelectionChange(selStart: Int, selEnd: Int) {
                      lynxContext.eventEmitter.sendCustomEvent(
                        LynxDetailEvent(
                          sign,
                          "selection"
                        ).apply {
                          addDetail("selectionStart", selStart)
                          addDetail("selectionEnd", selEnd)
                        })
                }
            }

        }
        editText.hint = ""
        return  editText
    }

    @LynxProp(name = "placeholder")
    fun setPlaceholder(value: String?) {
        mPlaceholder = value
    }

    @LynxProp(name = "placeholder-font-size", defaultFloat = UNDEFINED_FLOAT)
    fun setPlaceholderTextSize(size: Float) {
        mPlaceholderFontSize = size
    }

    @LynxProp(name = PropsConstants.FONT_WEIGHT, defaultInt = StyleConstants.FONTWEIGHT_NORMAL)
    fun setFontWeight(fontWeightNumerical: Int?) {
       fontWeightNumerical?.let{
            mFontWeight = when (it) {
                StyleConstants.FONTWEIGHT_BOLD -> 700
                StyleConstants.FONTWEIGHT_NORMAL -> 400
                else -> (it - 1) * 100
            }
        }
    }

    @LynxProp(name = "placeholder-font-weight")
    fun setPlaceholderTextWeight(weight: Dynamic?) {
        if (weight == null) {
            mPlaceholderFontWeight = UNDEFINED_INT;
        } else {
            when (weight.type) {
                ReadableType.Int, ReadableType.Long, ReadableType.Number -> {
                    val fontWeight = when (weight.asInt()) {
                        StyleConstants.FONTWEIGHT_BOLD -> 700
                        StyleConstants.FONTWEIGHT_NORMAL -> 400
                        else -> (weight.asInt() - 1) * 100
                    }
                    mPlaceholderFontWeight = fontWeight
                }
                ReadableType.String -> {
                    mPlaceholderFontWeight = if (weight.asString().equals("bold")) {
                        700
                    } else if (weight.asString().equals("normal")) {
                        400
                    } else {
                        UNDEFINED_INT
                    }
                }
                else -> {
                    mPlaceholderFontWeight = UNDEFINED_INT
                }
            }
        }
    }

    @LynxProp(name = PropsConstants.FONT_STYLE, defaultInt = StyleConstants.FONTSTYLE_NORMAL)
    open fun setFontStyle(fontStyle: Int) {
        mFontStyle = when(fontStyle) {
            StyleConstants.FONTSTYLE_NORMAL -> Typeface.NORMAL
            StyleConstants.FONTSTYLE_ITALIC -> Typeface.ITALIC
            StyleConstants.FONTSTYLE_OBLIQUE -> Typeface.ITALIC
            else -> Typeface.NORMAL
        }
    }

    @LynxProp(name = "placeholder-font-style")
    fun setPlaceholderFontStyle(fontStyle: Int) {
        mPlaceholderFontStyle = when(fontStyle) {
            StyleConstants.FONTSTYLE_NORMAL -> Typeface.NORMAL
            StyleConstants.FONTSTYLE_ITALIC -> Typeface.ITALIC
            StyleConstants.FONTSTYLE_OBLIQUE -> Typeface.ITALIC
            else -> UNDEFINED_INT
        }
    }

    @LynxProp(name = PropsConstants.FONT_FAMILY)
    open fun setFontFamily(fontFamily: String?) {
        fontFamily?.let {
            mFontFamilyName = it
        }
    }

    @LynxProp(name = "placeholder-font-family")
    fun setPlaceholderFontFamily(fontFamilyName: String?) {
        mPlaceholderFontFamilyName = fontFamilyName
    }

    @LynxProp(name = PropsConstants.COLOR, defaultInt = Color.BLACK)
    fun setFontColor(color: Dynamic) {
        when (color.type) {
            ReadableType.Int, ReadableType.Long -> mView.setTextColor(color.asInt())
            ReadableType.String -> mView.setTextColor(ColorUtils.parse(color.asString()))
            else -> {}
        }
    }

    @LynxProp(name = "placeholder-color")
    fun setPlaceholderColor(color: Dynamic) {
        mPlaceholderFontColor = when (color.type) {
            ReadableType.Int, ReadableType.Long -> color.asInt()
            ReadableType.String -> ColorUtils.parse(color.asString())
            else -> {
                ColorUtils.parse("#3c433c")
            }
        }
    }

    @LynxProp(name = PropsConstants.CARET_COLOR)
    fun setCursorColor(color: String?) {
        color?.let {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                mView.textCursorDrawable?.let { drawable ->
                    drawable.setTint(ColorUtils.parse(color))
                    mView.textCursorDrawable = drawable
                }
            }
        }
    }

    @LynxProp(name = PropsConstants.LINE_HEIGHT, defaultFloat = 0f)
    fun setLineHeight(lineHeight: Float) {
        // TODO(@xiamengfei.moonface): using CustomLineSpan to implement line-height
        // mLineHeight = lineHeight
    }

    @LynxProp(name = PropsConstants.LINE_SPACING)
    open fun setLineSpacing(lineSpacing: Dynamic) {
        if (lineSpacing.type == ReadableType.String) {
            mLineSpacing = UnitUtils.toPxWithDisplayMetrics(
                lineSpacing.asString(), 0f, 0f, 0f, 0f, -1.0f, lynxContext.screenMetrics
            )
        } else if (lineSpacing.type == ReadableType.Number || lineSpacing.type == ReadableType.Int) {
            mLineSpacing = PixelUtils.dipToPx(lineSpacing.asDouble())
        }
    }

    @LynxProp(name = PropsConstants.LETTER_SPACING, defaultFloat = 0f)
    fun setLetterSpacing(value: Float) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            if (mView.textSize != 0f) {
                mView.letterSpacing = value / mView.textSize
            }
        }
    }

    @LynxProp(name = PropsConstants.TEXT_ALIGN, defaultInt = StyleConstants.TEXTALIGN_LEFT)
    fun setTextAlign(align: Int) {
        val customSetting = customTextAlignSetting(align)
        when (align) {
            StyleConstants.TEXTALIGN_LEFT -> mView.gravity = (Gravity.LEFT or customSetting)
            StyleConstants.TEXTALIGN_RIGHT -> mView.gravity = (Gravity.RIGHT or customSetting)
            StyleConstants.TEXTALIGN_CENTER -> mView.gravity = (Gravity.CENTER_HORIZONTAL or customSetting)
        }
    }

    @LynxProp(name = PropsConstants.DRIECTION, defaultInt = StyleConstants.DIRECTION_LTR)
    override fun setLynxDirection(direction: Int) {
        this.mLynxDirection = direction
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            when (this.mLynxDirection) {
                StyleConstants.DIRECTION_NORMAL -> mView.textDirection =
                    View.TEXT_DIRECTION_LOCALE
                StyleConstants.DIRECTION_LTR -> mView.textDirection = View.TEXT_DIRECTION_LTR
                StyleConstants.DIRECTION_RTL, StyleConstants.DIRECTION_LYNXRTL -> mView.textDirection =
                    View.TEXT_DIRECTION_RTL
            }
        }
    }

    @LynxProp(name = "confirm-type")
    fun setConfirmType(value: String?) {
      if (isTextArea()) {
        // can not apply both confirm-type and type on textarea
        mView.isSingleLine = false
        mView.inputType = InputType.TYPE_CLASS_TEXT
        mView.setHorizontallyScrolling(false)
        mView.maxLines = mView.maxLines
      }
        when(value ?: "done") {
            "next" -> mView.imeOptions = EditorInfo.IME_ACTION_NEXT
            "search"-> mView.imeOptions = EditorInfo.IME_ACTION_SEARCH
            "send" -> mView.imeOptions = EditorInfo.IME_ACTION_SEND
            "done" -> mView.imeOptions = EditorInfo.IME_ACTION_DONE
            "go" -> mView.imeOptions = EditorInfo.IME_ACTION_GO
        }
    }

    @LynxProp(name = "type")
    fun setInputType(value: String?) {
            val imeOptions = mView.imeOptions;
            when (value ?: "text") {
                LynxUIBaseInput.TYPE_NUMBER ->  mView.inputType = InputType.TYPE_CLASS_NUMBER
                LynxUIBaseInput.TYPE_TEXT -> mView.inputType = InputType.TYPE_CLASS_TEXT
                LynxUIBaseInput.TYPE_PASSWORD -> mView.inputType = InputType.TYPE_TEXT_VARIATION_PASSWORD
                LynxUIBaseInput.TYPE_DIGIT -> mView.inputType = InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_DECIMAL
                LynxUIBaseInput.TYPE_TEL ->  mView.inputType = InputType.TYPE_CLASS_PHONE
                LynxUIBaseInput.TYPE_EMAIL -> mView.inputType = InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS
            }
            val selectionStart = mView.selectionStart
            if (value != LynxUIBaseInput.TYPE_PASSWORD) {
                mView.transformationMethod = SingleLineTransformationMethod.getInstance()
            } else {
                mView.transformationMethod = PasswordTransformationMethod.getInstance()
            }
            mView.setSelection(selectionStart)
            mView.inputType = mView.inputType or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
            if (isTextArea()) {
                mView.isSingleLine = false
                mView.imeOptions = imeOptions
            }
    }

    @LynxProp(name = "maxlength")
    fun setMaxLength(maxLength: Dynamic?) {
            when (maxLength?.type) {
                ReadableType.String -> {
                    maxLengthInputFilter = InputFilter.LengthFilter(maxLength.asString().toInt())
                }
                ReadableType.Long, ReadableType.Int, ReadableType.Number -> {
                    maxLengthInputFilter = InputFilter.LengthFilter(maxLength.asInt())
                }
                else -> {
                }
            }
    }

    @LynxProp(name = "input-filter")
    fun setInputFilter(value: String?) {
        mInputFilterRegex = value;
    }

    @LynxProp(name = "disabled", defaultBoolean = false)
    fun setDisable(disabled: Boolean) {
        mView.isEnabled = !disabled
        mView.isFocusable = !disabled
        mView.isFocusableInTouchMode = !disabled
        userInteractionEnabled = !disabled
    }

    @LynxProp(name = "readonly", defaultBoolean = false)
    fun setIsReadOnly(isReadOnly: Boolean) {
      mReadonly = isReadOnly;
    }

    @LynxProp(name = "show-soft-input-on-focus", defaultBoolean = true)
    fun setShowSoftInputOnFocus(isShowSoftInputOnFocus: Boolean) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            val context = ContextUtils.getActivity(lynxContext)
            if (context is Activity) {
                if (!isShowSoftInputOnFocus) {
                    mUseCustomKeyboard = true
                    mSoftInputModeStateStash =
                        context.window.attributes.softInputMode and WindowManager.LayoutParams.SOFT_INPUT_MASK_STATE
                    val currentSoftInputModeExpectState =
                        context.window.attributes.softInputMode xor WindowManager.LayoutParams.SOFT_INPUT_MASK_STATE
                    val newSoftInputMode =
                        currentSoftInputModeExpectState or WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN
                    context.window.setSoftInputMode(newSoftInputMode)
                    mView.showSoftInputOnFocus = false
                } else {
                    mUseCustomKeyboard = false
                    val currentSoftInputModeExpectState =
                        context.window.attributes.softInputMode xor WindowManager.LayoutParams.SOFT_INPUT_MASK_STATE
                    val newSoftInputMode =
                        currentSoftInputModeExpectState or mSoftInputModeStateStash
                    context.window.setSoftInputMode(newSoftInputMode)
                    mView.showSoftInputOnFocus = true
                }
            }
        }
    }

    @LynxProp(name = "android-fullscreen-mode", defaultBoolean = true)
    fun setKeyBoardFullscreenMode(isFullscreenMode: Boolean) {
        if (!isFullscreenMode) {
            mView.imeOptions = mView.imeOptions or EditorInfo.IME_FLAG_NO_FULLSCREEN or EditorInfo.IME_FLAG_NO_EXTRACT_UI
        } else {
            mView.imeOptions = mView.imeOptions and EditorInfo.IME_FLAG_NO_FULLSCREEN.inv() and EditorInfo.IME_FLAG_NO_EXTRACT_UI.inv()
        }
    }

    @LynxProp(name = "android-set-soft-input-mode")
    fun setSoftInputMode(value: String) {
        var inputMode =  WindowManager.LayoutParams.SOFT_INPUT_ADJUST_UNSPECIFIED
        if ("unspecified" == value) {
            inputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_UNSPECIFIED;
        } else if ("nothing" == value) {
            inputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING
        } else if ("pan" == value) {
            inputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN
        } else if ("resize" == value) {
            inputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
        }
        mView.updateInputMode(inputMode)
    }

    override fun afterPropsUpdated(props: StylesDiffMap?) {
        super.afterPropsUpdated(props)
        setFont()
        setPlaceholderFont()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && !mLineSpacing.equals(-1f) ) {
//            TODO(xiamengfei.moonface): Impl line height with custom Span.
            val fontHeight: Int = mView.paint.getFontMetricsInt(null)
            val lineHeight = max(fontHeight, fontHeight + mLineSpacing.toInt())
            mView.lineHeight = lineHeight
        }
    }

    override fun onLayoutUpdated() {
        super.onLayoutUpdated()
        mView.setPadding(mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom)
    }

    protected open fun setFont() {
        var typeface: Typeface? = null
        // get specific font-family
        if (mFontFamilyName != null) {
            typeface = TypefaceCache.getTypeface(lynxContext, mFontFamilyName, mFontStyle)
        }

        // get font-face in cache
        if (typeface == null) {
            typeface = FontFaceManager.getInstance()
                    .getTypeface(
                            lynxContext,
                            mFontFamilyName ?: "",
                            mFontStyle,
                            null
                    )
        }

        // get default font
        if (typeface == null) {
            typeface =  Typeface.create(mView.typeface, mFontStyle)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (typeface != null) {
                if (typeface.weight != mFontWeight) {
                    typeface = Typeface.create(typeface, mFontWeight, mFontStyle == Typeface.ITALIC)
                }
            }
        }

        mView.typeface = typeface
        mView.setTextSize(TypedValue.COMPLEX_UNIT_PX, mFontSize)
    }

    protected open fun setPlaceholderFont() {
        if (mPlaceholder == null) {
            mView.hint = ""
        } else {
            var placeholderFontWeight = mPlaceholderFontWeight
            if (placeholderFontWeight == UNDEFINED_INT) {
                placeholderFontWeight = mFontWeight
            }
            var placeholderFontStyle = mPlaceholderFontStyle
            if (placeholderFontStyle == UNDEFINED_INT) {
                placeholderFontStyle = mFontStyle
            }
            var placeholderFontFamilyName = mPlaceholderFontFamilyName
            if (placeholderFontFamilyName == null) {
                placeholderFontFamilyName = mFontFamilyName
            }
            var placeholderFontSize = mPlaceholderFontSize
            if (placeholderFontSize == UNDEFINED_FLOAT) {
                placeholderFontSize = mFontSize
            }
            var typeface: Typeface? = null
            // get specific font-family
            if (placeholderFontFamilyName != null) {
                typeface = TypefaceCache.getTypeface(lynxContext, placeholderFontFamilyName, placeholderFontStyle)
            }
            // get font-face in cache
            if (typeface == null) {
                typeface = FontFaceManager.getInstance()
                        .getTypeface(
                                lynxContext,
                                placeholderFontFamilyName ?: "",
                                placeholderFontStyle,
                                null
                        )
            }
            // get default font
            if (typeface == null) {
                typeface =  Typeface.create(mView.typeface, placeholderFontStyle!!)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                if (typeface != null) {
                    if (typeface.weight != placeholderFontWeight) {
                        typeface = Typeface.create(typeface, placeholderFontWeight, placeholderFontStyle == Typeface.ITALIC)
                    }
                }
            }

            val placeholderSpan = SpannableString(mPlaceholder)
            val length = placeholderSpan.length
            placeholderSpan.setSpan(AbsoluteSizeSpan(placeholderFontSize.toInt(), false), 0, length, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
            if (typeface != null) {
                placeholderSpan.setSpan(FontFamilySpan(typeface), 0, length, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
            }
            mView.hint = placeholderSpan
            mView.setHintTextColor(mPlaceholderFontColor)

        }
    }

    override fun onNodeReady() {
        super.onNodeReady()
        val textLayout = LynxInputUtils().getLayoutInEditText(mView.text.toString(),
            mView,
            Int.MAX_VALUE,
            Int.MAX_VALUE)

        triggerUpdateLayout(textLayout.height)
    }

    open fun triggerUpdateLayout(updatedHeight: Int) {
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

    open fun customTextAlignSetting(align: Int) : Int = 0

    override fun isFocusable(): Boolean {
        return mView.isFocusable
    }

    override fun onFocusChanged(hasFocus: Boolean, isFocusTransition: Boolean) {
        if (!isFocusTransition ) {
            if (hasFocus) {
                mView.requestFocus()
                if (!mUseCustomKeyboard) {
                    // Double check in onFocusChangeListener
                    showSoftInput()
                }
            } else {
                mView.clearFocus()
                hideSoftInput()
            }
        }
    }

    @LynxUIMethod
    fun setValue(params: ReadableMap?, callback: Callback?) {
        if (params == null) {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, "Param is not a map.")
            return
        }

        val text = if (params.hasKey("value")) params.getString("value") else ""
        val cursor = if (params.hasKey("cursor")) params.getInt("cursor") else - 1
        if (mView.text == null) {
            mView.setText(text)
        } else {
            mView.text!!.replace(0, mView.text!!.length, text)
        }
        if (cursor != -1) {
            mView.setSelection(cursor)
        }
        callback?.invoke(LynxUIMethodConstants.SUCCESS)
    }

    @LynxUIMethod
    fun getValue(params: ReadableMap?, callback: Callback?) {
        var composing = false
        mView.text?.let {
            composing = mView.inputConnection()?.hasComposingText(it) == true
        }
        val result = JavaOnlyMap()
        var text = ""
        mView.text ?. let { 
          text = mView.text.toString();
        }
        var selectionStart = -1
        if (mView.isFocused) {
          selectionStart = mView.selectionStart
        }
        var selectionEnd = -1
        if (mView.isFocused) {
          selectionEnd = mView.selectionEnd
        }
        result["value"] = text;
        result["selectionStart"] = selectionStart
        result["selectionEnd"] = selectionEnd
        result["isComposing"] = composing
        callback?.invoke(LynxUIMethodConstants.SUCCESS, result)
    }

    @LynxUIMethod
    fun setSelectionRange(params: ReadableMap?, callback: Callback?) {
        if (params == null) {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, "Param is not a map.")
            return
        }

        var selectionStart = -1
        var selectionEnd = -1
        if (params.hasKey("selectionStart")) {
            selectionStart = params.getInt("selectionStart")
        }
        if (params.hasKey("selectionEnd")) {
            selectionEnd = params.getInt("selectionEnd")
        }
        val textLength = if (mView.text != null) {
            mView.text!!.length
        } else {
            -1
        }
        if (textLength == -1 || selectionStart > textLength || selectionEnd > textLength ||
            selectionStart < 0 || selectionEnd < 0) {
            callback?.invoke(LynxUIMethodConstants.PARAM_INVALID, "Range does not meet expectations.")
            return
        }
        mView.setSelection(selectionStart, selectionEnd)
        callback?.invoke(LynxUIMethodConstants.SUCCESS, "Success.")
    }

    @LynxUIMethod
    fun focus(params: ReadableMap?, callback: Callback?) {
        if (mView.requestFocus()) {
            if (mUseCustomKeyboard) {
                hideSoftInput()
            } else if (showSoftInput()){
                callback?.invoke(LynxUIMethodConstants.SUCCESS)
            } else {
                callback?.invoke(LynxUIMethodConstants.UNKNOWN, "fail to show keyboard")
            }
        } else {
            callback?.invoke(LynxUIMethodConstants.UNKNOWN, "fail to focus")
        }
    }

    @LynxUIMethod
    fun blur(params: ReadableMap?, callback: Callback?) {
        mView.clearFocus()
        if (!mUseCustomKeyboard) {
            hideSoftInput()
        }
        callback?.invoke(LynxUIMethodConstants.SUCCESS)
    }

    @SuppressLint("WrongConstant")
    private fun showSoftInput():Boolean {
        val imm = lynxContext.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        return imm.showSoftInput(mView,
            InputMethodManager.SHOW_IMPLICIT, null)
    }

    @SuppressLint("WrongConstant")
    private fun hideSoftInput() {
            val imm =
                lynxContext.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            imm.hideSoftInputFromWindow(mView.windowToken, 0)
    }

    protected fun onConfirm() {
        lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(
                sign,
                "confirm"
            ).apply {
                addDetail("value", mView.text?.toString())
            })
    }
  
    open fun isTextArea(): Boolean {
      return false;
    }
}
