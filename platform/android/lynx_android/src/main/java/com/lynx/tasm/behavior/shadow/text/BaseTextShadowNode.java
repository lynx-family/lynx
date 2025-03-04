/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_ITALIC;
import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.FONTSTYLE_OBLIQUE;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_100;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_200;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_300;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_400;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_500;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_600;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_700;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_800;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_900;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_BOLD;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_START;
import static com.lynx.tasm.behavior.StyleConstants.TEXTOVERFLOW_CLIP;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_LINETHROUGH;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_NONE;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_UNDERLINE;
import static com.lynx.tasm.behavior.StyleConstants.WHITESPACE_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.WHITESPACE_NOWRAP;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.FIRST_CHAR_RTL_STATE_LTR;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.FIRST_CHAR_RTL_STATE_RTL;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.INLINE_BLOCK_PLACEHOLDER;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.INLINE_IMAGE_PLACEHOLDER;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.NOT_SET;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.TEXT_VERTICAL_ALIGN_CUSTOM;

import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.Typeface;
import android.os.Build;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import androidx.annotation.ColorInt;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.ShadowData;
import com.lynx.tasm.behavior.ui.background.BackgroundLinearGradientLayer;
import com.lynx.tasm.behavior.ui.background.BackgroundRadialGradientLayer;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.utils.UnicodeFontUtils;
import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.PixelUtils;
import java.lang.reflect.Method;
import java.text.Bidi;
import java.util.List;
import java.util.Set;

public class BaseTextShadowNode extends ShadowNode {
  private static final String TAG = "lynx_BaseTextShadowNode";
  private TextAttributes mTextAttributes;
  private boolean mEnableFontScaling = false;
  private boolean mForceFakeBold = false;
  private boolean mUseWebLineHeight = false;
  private float mOriginLineHeight = MeasureUtils.UNDEFINED;
  private boolean mEnableTextRefactor = false;
  private boolean mEnableNewClipMode = false;
  private boolean mEnableTextBoringLayout = false;
  // for css word-break style
  protected int mWordBreakStyle = StyleConstants.WORDBREAK_NORMAL;
  // Use bitmap shader to draw linear gradient
  private boolean mEnableBitmapGradient = false;
  /**
   * Since minSdk is 16 use raw value
   * 0 -> Layout.BREAK_STRATEGY_SIMPLE
   * 1 -> Layout.BREAK_STRATEGY_HIGH_QUALITY
   * 2 -> Layout.BREAK_STRATEGY_BALANCED
   */
  private static final int WORD_BREAK_STRATEGY_SIMPLE = 0;
  private static final int WORD_BREAK_STRATEGY_HIGH_QUALITY = 1;
  private static final int WORD_BREAK_STRATEGY_BALANCED = 2;
  // for StaticLayout.BreakStrategy
  protected int mWordBreakStrategy = WORD_BREAK_STRATEGY_SIMPLE;
  protected boolean mNeedDrawStroke;
  @ColorInt protected int mSelectionColor = 0;
  protected boolean mEnableEmojiCompat;
  protected static Method sEmojiProcess = null;
  protected static Object sEmojiCompatInst = null;
  private static boolean sSupportEmojiCompat = true;
  protected @Nullable String mText = null;

  public BaseTextShadowNode() {
    mTextAttributes = new TextAttributes();
  }

  @Override
  public void setContext(LynxContext context) {
    super.setContext(context);
    mEnableTextRefactor = getContext().isTextRefactorEnabled();
    mEnableNewClipMode = getContext().isNewClipModeEnabled();
    mEnableTextBoringLayout = getContext().isTextBoringLayoutEnabled();
    mTextAttributes.setIncludePadding(getContext().getDefaultTextIncludePadding());
    mTextAttributes.setFontSize(
        Math.round(PixelUtils.dipToPx(14, getContext().getScreenMetrics().density)));
  }

  public TextAttributes getTextAttributes() {
    return mTextAttributes;
  }

  protected boolean isTextRefactorEnabled() {
    return mEnableTextRefactor;
  }

  protected boolean isTextBoringLayoutEnabled() {
    return mEnableTextBoringLayout;
  }

  @LynxProp(name = PropsConstants.WHITE_SPACE, defaultInt = StyleConstants.WHITESPACE_NORMAL)
  public void setWhiteSpace(int whiteSpace) {
    mTextAttributes.mWhiteSpace = whiteSpace;
    markDirty();
  }

  @LynxProp(name = PropsConstants.INCLUDE_FONT_PADDING)
  public void setIncludeFontPadding(boolean includeFontPadding) {
    mTextAttributes.setIncludePadding(includeFontPadding);
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_OVERFLOW, defaultInt = TEXTOVERFLOW_CLIP)
  public void setTextOverflow(int textOverflow) {
    mTextAttributes.mTextOverflow = textOverflow;
    markDirty();
  }

  @LynxProp(name = PropsConstants.FONT_WEIGHT, defaultInt = FONTWEIGHT_NORMAL)
  public void setFontWeight(int fontWeightNumeric) {
    int fontWeight = fontWeightNumeric;
    int fontStyle = Typeface.NORMAL;
    switch (fontWeightNumeric) {
      case FONTWEIGHT_BOLD:
      case FONTWEIGHT_500:
      case FONTWEIGHT_600:
      case FONTWEIGHT_700:
      case FONTWEIGHT_800:
      case FONTWEIGHT_900:
        fontStyle = Typeface.BOLD;
        break;
      case FONTWEIGHT_NORMAL:
      case FONTWEIGHT_100:
      case FONTWEIGHT_200:
      case FONTWEIGHT_300:
      case FONTWEIGHT_400:
        fontStyle = Typeface.NORMAL;
        break;
      default:
        break;
    }
    if (fontWeight != mTextAttributes.mFontWeight) {
      mTextAttributes.mFontWeight = fontWeight;
      mTextAttributes.mFontStyle = fontStyle;
      markDirty();
    }
  }

  @LynxProp(name = PropsConstants.FONT_STYLE, defaultInt = FONTSTYLE_NORMAL)
  public void setFontStyle(int fontStyle) {
    if (fontStyle == FONTSTYLE_NORMAL && mTextAttributes.mFontStyle != Typeface.NORMAL) {
      mTextAttributes.mFontStyle = Typeface.NORMAL;
      markDirty();
    }
    if ((fontStyle == FONTSTYLE_ITALIC || fontStyle == FONTSTYLE_OBLIQUE)
        && mTextAttributes.mFontStyle != Typeface.ITALIC) {
      mTextAttributes.mFontStyle = Typeface.ITALIC;
      markDirty();
    }
  }

  @LynxProp(name = PropsConstants.FONT_FAMILY)
  public void setFontFamily(String fontFamily) {
    if (fontFamily == null && !TextUtils.isEmpty(mTextAttributes.mFontFamily)) {
      mTextAttributes.mFontFamily = null;
      markDirty();
      return;
    }
    if (fontFamily != null && !fontFamily.equals(mTextAttributes.mFontFamily)) {
      mTextAttributes.mFontFamily = fontFamily;
      markDirty();
    }
  }

  @LynxProp(name = "use-web-line-height")
  public void setUseWebLineHeight(boolean useWebLineHeight) {
    if (mUseWebLineHeight != useWebLineHeight) {
      mUseWebLineHeight = useWebLineHeight;
      if (mOriginLineHeight != MeasureUtils.UNDEFINED) {
        setLineHeight(mOriginLineHeight);
      }
    }
  }

  @LynxProp(name = "custom-baseline-shift")
  public void setBaselineShift(String shift) {
    try {
      if (shift.contains("px")) {
        String number = shift.substring(0, shift.indexOf("px")).trim();
        float n = Float.parseFloat(number);
        mTextAttributes.mBaselineShift = PixelUtils.dipToPx(n);
        mTextAttributes.mTextVerticalAlign = TEXT_VERTICAL_ALIGN_CUSTOM;
      } else if (shift.contains("%")) {
        String number = shift.substring(0, shift.indexOf("%")).trim();
        mTextAttributes.mBaselineShift =
            Float.parseFloat(number) * 0.01f * mTextAttributes.mFontSize;
        mTextAttributes.mTextVerticalAlign = TEXT_VERTICAL_ALIGN_CUSTOM;
      } else {
        String number = shift.trim();
        float n = Float.parseFloat(number);
        mTextAttributes.mBaselineShift = n * mTextAttributes.mFontSize;
        mTextAttributes.mTextVerticalAlign = TEXT_VERTICAL_ALIGN_CUSTOM;
      }
    } catch (Exception e) {
      LLog.e("BaseTextShadowNode", e.toString());
      mTextAttributes.mBaselineShift = 0.f;
      mTextAttributes.mTextVerticalAlign = NOT_SET;
    } finally {
      markDirty();
    }
  }

  protected void setLineHeightInternal(float lineHeight) {
    mOriginLineHeight = lineHeight;
    if (mEnableFontScaling && mContext != null) {
      lineHeight = lineHeight * mContext.getResources().getConfiguration().fontScale;
    }
    if (mTextAttributes.mLineHeight != lineHeight) {
      mTextAttributes.mLineHeight = lineHeight;
      markDirty();
    }
  }

  @LynxProp(name = PropsConstants.LINE_HEIGHT, defaultFloat = MeasureUtils.UNDEFINED)
  public void setLineHeight(float lineHeight) {
    // There is no way to support inline line-height based on pure Android text layout.
    // So let's make it clear, Lynx won't support line-height on inline-text component.
    if (!isTextRefactorEnabled()) {
      setLineHeightInternal(lineHeight);
    }
  }

  /**
   * Notice: fontSize set by native is a pixel unit, a raw pixel without scale, so
   * you handle sp by yourself if you want to apply scaled pixel.
   *
   * @param fontSize raw pixel
   */
  @LynxProp(name = PropsConstants.FONT_SIZE, defaultFloat = MeasureUtils.UNDEFINED)
  public void setFontSize(float fontSize) {
    if (fontSize != MeasureUtils.UNDEFINED) {
      fontSize = (float) Math.round(fontSize);
    }
    if (mEnableFontScaling && mContext != null) {
      fontSize = fontSize * mContext.getResources().getConfiguration().fontScale;
    }
    if (mTextAttributes.mFontSize != (int) fontSize) {
      mTextAttributes.mFontSize = (int) fontSize;
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.ENABLE_FONT_SCALING)
  public void setEnableFontScaling(String enable) {
    setEnableFontScaling(Boolean.parseBoolean(enable));
  }

  @Deprecated
  public void setColor(int color) {
    mTextAttributes.mFontColor = color;
    markDirty();
  }

  @LynxProp(name = PropsConstants.COLOR)
  public void setColor(Dynamic color) {
    if (color.getType() == ReadableType.Array) {
      mTextAttributes.mFontColor = null;
      setGradientColor(color.asArray());
    } else if (color.getType() == ReadableType.Int) {
      mTextAttributes.mFontColor = color.asInt();
      mTextAttributes.mTextGradient = null;
    } else if (color.getType() == ReadableType.Long) {
      mTextAttributes.mFontColor = (int) color.asLong();
      mTextAttributes.mTextGradient = null;
    } else {
      mTextAttributes.mFontColor = null;
      mTextAttributes.mTextGradient = null;
    }
    markDirty();
  }

  private void setGradientColor(ReadableArray args) {
    if (args.size() < 2 || args.getType(1) != ReadableType.Array) {
      mTextAttributes.mTextGradient = null;
      return;
    }

    // gradient type is uint32_t in c++, so we need to convert it to long
    long type = args.getLong(0);
    if (type == StyleConstants.BACKGROUND_IMAGE_LINEAR_GRADIENT) {
      mTextAttributes.mTextGradient = new BackgroundLinearGradientLayer(args.getArray(1));
      if (mEnableBitmapGradient) {
        mTextAttributes.mTextGradient.setEnableBitmapGradient(true);
      }
    } else if (type == StyleConstants.BACKGROUND_IMAGE_RADIAL_GRADIENT) {
      mTextAttributes.mTextGradient = new BackgroundRadialGradientLayer(args.getArray(1));
    } else {
      mTextAttributes.mTextGradient = null;
    }
  }

  @LynxProp(name = PropsConstants.LETTER_SPACING, defaultFloat = MeasureUtils.UNDEFINED)
  public void setLetterSpacing(float letterSpacing) {
    mTextAttributes.mLetterSpacing = letterSpacing;
    markDirty();
  }

  @LynxProp(name = PropsConstants.LINE_SPACING, defaultFloat = 0)
  public void setLineSpacing(float lineSpacing) {
    mTextAttributes.mLineSpacing = lineSpacing;
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_SHADOW)
  public void setTextShadow(@Nullable ReadableArray textShadow) {
    mTextAttributes.mTextShadow = null;
    if (null == textShadow) {
      return;
    }
    List<ShadowData> boxShadows = ShadowData.parseShadow(textShadow);
    if (boxShadows.size() == 0) {
      return;
    }
    ShadowData boxShadow = boxShadows.get(0);
    mTextAttributes.mTextShadow = boxShadow;
  }

  @Deprecated
  public void setTextDecoration(@StyleConstants.TextDecoration int textDecoration) {
    mTextAttributes.mTextDecoration = textDecoration;
    mTextAttributes.mTextDecorationStyle = 4; // 4 means solid
    mTextAttributes.mTextDecorationColor = Color.BLACK;
    markDirty();
    LLog.w("text-decoration", "setTextDecoration(int) is deprecated");
  }

  @LynxProp(name = PropsConstants.TEXT_DECORATION)
  public void setTextDecoration(ReadableArray textDecoration) {
    if (textDecoration == null || textDecoration.size() != 3) {
      mTextAttributes.mTextDecoration = TEXT_DECORATION_NONE;
      // text-decoration-style, 4 means solid
      mTextAttributes.mTextDecorationStyle = 4;
      mTextAttributes.mTextDecorationColor = 0;
      markDirty();
      return;
    }
    int textDecorationLine = textDecoration.getInt(0);
    int textDecorationStyle = textDecoration.getInt(1);
    int textDecorationColor = textDecoration.getInt(2);
    mTextAttributes.mTextDecoration = textDecorationLine;
    mTextAttributes.mTextDecorationStyle = textDecorationStyle;
    mTextAttributes.mTextDecorationColor = textDecorationColor;
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_STROKE_WIDTH, defaultFloat = 0)
  public void setTextStrokeWidth(float textStrokeWidth) {
    mTextAttributes.setTextStrokeWidth(textStrokeWidth);
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_STROKE_COLOR)
  public void setTextStrokeColor(Dynamic color) {
    if (color.getType() == ReadableType.Int) {
      mTextAttributes.setTextStrokeColor(color.asInt());
    } else if (color.getType() == ReadableType.Long) {
      mTextAttributes.setTextStrokeColor((int) color.asLong());
    } else {
      mTextAttributes.setTextStrokeColor(0);
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_ALIGN, defaultInt = TEXTALIGN_START)
  public void setTextAlign(int textAlign) {
    mTextAttributes.mTextAlign = textAlign;
    markDirty();
  }

  @LynxProp(name = PropsConstants.DRIECTION, defaultInt = DIRECTION_NORMAL)
  public void setDirection(int direction) {
    mTextAttributes.mDirection = direction;
    markDirty();
  }

  @LynxProp(name = "text-vertical-align")
  public void setTextVerticalAlign(String textVerticalAlign) {
    if ("top".equals(textVerticalAlign)) {
      mTextAttributes.mTextVerticalAlign = TextAttributes.TEXT_VERTICAL_ALIGN_TOP;
    } else if ("center".equals(textVerticalAlign)) {
      mTextAttributes.mTextVerticalAlign = TextAttributes.TEXT_VERTICAL_ALIGN_CENTER;
    } else if ("bottom".equals(textVerticalAlign)) {
      mTextAttributes.mTextVerticalAlign = TextAttributes.TEXT_VERTICAL_ALIGN_BOTTOM;
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_MAXLINE)
  public void setTextMaxLine(String maxLine) {
    try {
      mTextAttributes.mMaxLineCount = Integer.parseInt(maxLine);
    } catch (Throwable t) {
      mTextAttributes.mMaxLineCount = TextAttributes.NOT_SET;
      LLog.e(TAG, "setTextMaxLine with invalid value:" + maxLine);
    } finally {
      if (mTextAttributes.mMaxLineCount < 0) {
        mTextAttributes.mMaxLineCount = TextAttributes.NOT_SET;
      }
      markDirty();
    }
  }

  @LynxProp(name = PropsConstants.TEXT_MAXLENGTH)
  public void setTextMaxLength(String textMaxLength) {
    try {
      mTextAttributes.mMaxTextLength = Integer.valueOf(textMaxLength);
      markDirty();
    } catch (Throwable t) {
      mTextAttributes.mMaxTextLength = TextAttributes.NOT_SET;
      LLog.e(TAG, "setTextMaxLength with invalid value:" + textMaxLength);
    } finally {
      if (mTextAttributes.mMaxTextLength < 0) {
        mTextAttributes.mMaxTextLength = TextAttributes.NOT_SET;
      }
    }
  }

  @LynxProp(name = PropsConstants.WORD_BREAK_STRATEGY)
  public void setWordBreakStrategy(int value) {
    if (value == StyleConstants.WORDBREAK_KEEPALL || value == StyleConstants.WORDBREAK_BREAKALL) {
      if (mEnableNewClipMode) {
        mWordBreakStyle = value;
      } else {
        mWordBreakStrategy = WORD_BREAK_STRATEGY_HIGH_QUALITY;
      }
    } else if (value == StyleConstants.WORDBREAK_NORMAL) {
      mWordBreakStrategy = WORD_BREAK_STRATEGY_BALANCED;
    } else {
      mWordBreakStrategy = WORD_BREAK_STRATEGY_SIMPLE;
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT_FAKE_BOLD)
  public void setTextFakeBold(boolean fakeBold) {
    mForceFakeBold = fakeBold;
    markDirty();
  }

  public int getTypefaceStyle() {
    return mTextAttributes.getTypefaceStyle();
  }

  protected void generateStyleSpan(SpannableStringBuilder sb, List<SetSpanOperation> ops) {
    int start = sb.length();

    if (getChildCount() == 0 && mText != null) {
      appendText(sb, mText, false);
    }
    for (int i = 0, length = getChildCount(); i < length; i++) {
      ShadowNode child = getChildAt(i);

      if (child instanceof RawTextShadowNode) {
        RawTextShadowNode rawTextShadowNode = (RawTextShadowNode) child;
        if (rawTextShadowNode.getText() != null) {
          appendText(sb, rawTextShadowNode.getText(), rawTextShadowNode.isPseudo());
        }
      } else if (child instanceof AbsInlineImageShadowNode) {
        // We make the image take up 1 character in the span and put a corresponding
        // character into
        // the text so that the image doesn't run over any following text.
        sb.append(INLINE_IMAGE_PLACEHOLDER);
        ((AbsInlineImageShadowNode) child)
            .generateStyleSpan(sb.length() - INLINE_IMAGE_PLACEHOLDER.length(), sb.length(), ops);
        mTextAttributes.mHasImageSpan = true;
      } else if (child instanceof NativeLayoutNodeRef) {
        sb.append(INLINE_BLOCK_PLACEHOLDER);
        ((NativeLayoutNodeRef) child)
            .generateStyleSpan(sb.length() - INLINE_BLOCK_PLACEHOLDER.length(), sb.length(), ops);
        mTextAttributes.mHasInlineViewSpan = true;
      } else if (child instanceof BaseTextShadowNode) {
        if (child instanceof InlineTruncationShadowNode) {
          continue;
        }
        BaseTextShadowNode shadowNode = ((BaseTextShadowNode) child);
        // when the inline node not set color but need text stroke, it will not create
        // foregroundColorSpan, so here set the font color from super node for create span to show
        // text stroke
        if (shadowNode.getTextAttributes().mFontColor == null
            && shadowNode.getTextAttributes().mTextStrokeWidth > 0) {
          if (getTextAttributes().mFontColor != null) {
            shadowNode.getTextAttributes().setFontColor(getTextAttributes().getFontColor());
          } else {
            shadowNode.getTextAttributes().setFontColor((int) Color.BLACK);
          }
        }
        shadowNode.generateStyleSpan(sb, ops);
        mTextAttributes.mHasImageSpan |= shadowNode.mTextAttributes.mHasImageSpan;
        mTextAttributes.mHasInlineViewSpan |= shadowNode.mTextAttributes.mHasInlineViewSpan;
        mNeedDrawStroke |= shadowNode.mNeedDrawStroke;
      } else if (child instanceof TextSelectionShadowNode) {
        mSelectionColor = ((TextSelectionShadowNode) child).getBackgroundColor();
      } else {
        throw new RuntimeException(
            "Unexpected view type nested under text node: " + child.getClass());
      }
    }

    int end = sb.length();
    if (end > start) {
      buildStyledSpan(start, end, ops);
    }
  }

  // Find truncated inline view in text
  protected void getNativeNodeTruncatedMap(CharSequence text, Set viewTruncated) {
    for (int i = 0; i < getChildCount(); ++i) {
      ShadowNode child = getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        NativeLayoutNodeRef layoutNode = ((NativeLayoutNodeRef) child);
        if (layoutNode.getSpanStart() >= text.length()
            || text.charAt(layoutNode.getSpanStart()) != INLINE_BLOCK_PLACEHOLDER.charAt(0)) {
          viewTruncated.add(layoutNode.getSignature());
        }
      } else if (child instanceof BaseTextShadowNode
          && !(child instanceof InlineTruncationShadowNode)) {
        ((BaseTextShadowNode) child).getNativeNodeTruncatedMap(text, viewTruncated);
      }
    }
  }

  protected void measureNativeNode(
      SpannableStringBuilder sb, MeasureParam param, MeasureContext ctx) {
    for (int i = 0; i < getChildCount(); ++i) {
      ShadowNode child = getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        NativeLayoutNodeRef layoutNode = ((NativeLayoutNodeRef) child);
        MeasureResult result = layoutNode.measureNativeNode(ctx, param);
        NativeLayoutNodeSpan[] nativeNodeSpans = sb.getSpans(
            layoutNode.getSpanStart(), layoutNode.getSpanEnd(), NativeLayoutNodeSpan.class);
        for (NativeLayoutNodeSpan nodeSpan : nativeNodeSpans) {
          nodeSpan.updateLayoutNodeSize((int) Math.ceil(result.getWidthResult()),
              (int) Math.ceil(result.getHeightResult()),
              (int) Math.ceil(result.getBaselineResult()));
        }
      } else if (child instanceof BaseTextShadowNode
          && !(child instanceof InlineTruncationShadowNode)) {
        ((BaseTextShadowNode) child).measureNativeNode(sb, param, ctx);
      }
    }
  }

  protected void alignNativeNode(Layout layout, SpannableStringBuilder sb, AlignParam param,
      AlignContext ctx, PointF textTranslateOffset) {
    for (int i = 0; i < getChildCount(); ++i) {
      ShadowNode child = getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        NativeLayoutNodeRef layoutNode = ((NativeLayoutNodeRef) child);
        if (layoutNode.getSpanStart() >= layout.getText().length()) {
          continue;
        }

        NativeLayoutNodeSpan[] nativeNodeSpans = sb.getSpans(
            layoutNode.getSpanStart(), layoutNode.getSpanEnd(), NativeLayoutNodeSpan.class);
        NativeLayoutNodeSpan layoutNodeSpan =
            nativeNodeSpans.length == 1 ? nativeNodeSpans[0] : null;

        AlignParam nParam = new AlignParam();
        int line = layout.getLineForOffset(layoutNode.getSpanStart());
        float leftOffset =
            layout.getPrimaryHorizontal(layoutNode.getSpanStart()) + textTranslateOffset.x;
        if (layout.isRtlCharAt(layoutNode.getSpanStart())) {
          leftOffset -= (layoutNodeSpan == null ? 0 : layoutNodeSpan.getWidth());
        }
        nParam.setLeftOffset(leftOffset);

        if (layoutNodeSpan != null) {
          nParam.setTopOffset(
              layoutNodeSpan.getYOffset(layout.getLineTop(line), layout.getLineBottom(line),
                  layout.getLineAscent(line), layout.getLineDescent(line))
              + textTranslateOffset.y);
        }

        layoutNode.alignNativeNode(ctx, nParam);
      } else if (child instanceof InlineTextShadowNode
          || child instanceof InlineTruncationShadowNode) {
        ((BaseTextShadowNode) child).alignNativeNode(layout, sb, param, ctx, textTranslateOffset);
      }
    }
  }

  private int wordBreakStyleToDecodeProperty(int wordBreak) {
    switch (wordBreak) {
      case StyleConstants.WORDBREAK_BREAKALL:
        return UnicodeFontUtils.DECODE_INSERT_ZERO_WIDTH_CHAR;
      case StyleConstants.WORDBREAK_KEEPALL:
        return UnicodeFontUtils.DECODE_CJK_INSERT_WORD_JOINER;
      default:
        return UnicodeFontUtils.DECODE_DEFAULT;
    }
  }

  private CharSequence getDecodedCharSequence(String text, boolean isPseudo) {
    return isPseudo
        ? UnicodeFontUtils.decodeCSSContent(text, wordBreakStyleToDecodeProperty(mWordBreakStyle))
        : UnicodeFontUtils.decode(text, wordBreakStyleToDecodeProperty(mWordBreakStyle));
  }

  protected CharSequence getCharSequence(String text, boolean isPseudo) {
    if (!mEnableEmojiCompat || !sSupportEmojiCompat) {
      return getDecodedCharSequence(text, isPseudo);
    }
    try {
      return (CharSequence) sEmojiProcess.invoke(
          sEmojiCompatInst, getDecodedCharSequence(text, isPseudo));
    } catch (Exception e) {
      LLog.w(TAG, "process emoji: " + e);
      return getDecodedCharSequence(text, isPseudo);
    }
  }

  protected void appendText(SpannableStringBuilder sb, String text, boolean isPseudo) {
    sb.append(getCharSequence(text, isPseudo));
  }

  // set text stroke
  protected void configTextStroke(ForegroundColorSpan span) {
    if (mTextAttributes.getTextStrokeWidth() > 0 && span != null
        && mTextAttributes.getTextStrokeColor() != 0) {
      span.setStrokeColor(mTextAttributes.getTextStrokeColor());
      span.setStrokeWidth(mTextAttributes.getTextStrokeWidth());
      mNeedDrawStroke = true;
    }
  }

  protected void buildStyledSpan(int start, int end, List<SetSpanOperation> ops) {
    // Do not do this as it will slow down layout.draw(), do it by TextPaint
    // instead.
    // You can do it with span while you want some effect like rich text.
    if (getTextAttributes().mFontColor != null) {
      ForegroundColorSpan foregroundColorSpan =
          new ForegroundColorSpan(getTextAttributes().mFontColor);
      configTextStroke(foregroundColorSpan);
      ops.add(new SetSpanOperation(start, end, foregroundColorSpan));
    }
    // The default text-decoration-style is 4 (solid), the default text-decoration-color
    // is font color, the default font color is 0xFF000000(black)
    if (mTextAttributes.mTextDecorationStyle != 4 || mTextAttributes.mTextDecorationColor != 0) {
      boolean underline = ((mTextAttributes.mTextDecoration & TEXT_DECORATION_UNDERLINE) != 0);
      boolean linethrough = ((mTextAttributes.mTextDecoration & TEXT_DECORATION_LINETHROUGH) != 0);
      if (underline || linethrough) {
        ops.add(new SetSpanOperation(start, end,
            new TextDecorationSpan(underline, linethrough, mTextAttributes.mTextDecorationStyle,
                mTextAttributes.mTextDecorationColor)));
      }
    } else { // if not set the color and style, we will use default span.
      if ((mTextAttributes.mTextDecoration & TEXT_DECORATION_LINETHROUGH) != 0) {
        ops.add(new SetSpanOperation(start, end, new LynxStrikethroughSpan()));
      }
      if ((mTextAttributes.mTextDecoration & TEXT_DECORATION_UNDERLINE) != 0) {
        ops.add(new SetSpanOperation(start, end, new LynxUnderlineSpan()));
      }
    }

    // Set text-vertical-align
    if (mTextAttributes.mTextVerticalAlign != NOT_SET
        && Build.VERSION.SDK_INT > Build.VERSION_CODES.P) {
      CustomBaselineShiftSpan span = new CustomBaselineShiftSpan(
          start, end, mTextAttributes.mTextVerticalAlign, mTextAttributes.mBaselineShift);
      ops.add(new SetSpanOperation(start, end, span));
    }

    if (getShadowStyle() != null
        && getShadowStyle().verticalAlign != StyleConstants.VERTICAL_ALIGN_DEFAULT) {
      InlineTextBaselineShiftSpan span = new InlineTextBaselineShiftSpan();
      span.setVerticalAlign(getShadowStyle().verticalAlign, getShadowStyle().verticalAlignLength);
      ops.add(new SetSpanOperation(start, end, span));
    }

    if (isNeedSetLineHeightSpan()) {
      ops.add(new SetSpanOperation(start, end,
          new CustomLineHeightSpan(mTextAttributes.mLineHeight, isTextRefactorEnabled(),
              mTextAttributes.mTextSingleLineVerticalAlign, isSingLineAndOverflowClip())));
    }

    if (null != mTextAttributes.mTextShadow) {
      ops.add(new SetSpanOperation(start, end, new ShadowStyleSpan(mTextAttributes.mTextShadow)));
    }

    // Set letter spacing
    if (getTextAttributes().mLetterSpacing != MeasureUtils.UNDEFINED
        && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      ops.add(new SetSpanOperation(
          start, end, new CustomLetterSpacingSpan(getTextAttributes().mLetterSpacing)));
    }

    // Set text font weight and font style
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && !mForceFakeBold) {
      ops.add(new SetSpanOperation(start, end,
          new CustomStyleSpan(getTextAttributes().mFontStyle, getTextAttributes().mFontWeight,
              getTextAttributes().mFontFamily)));
    } else {
      // fallback to lower level api
      if (getTextAttributes().mFontStyle == Typeface.BOLD
          || getTextAttributes().mFontStyle == Typeface.ITALIC
          || getTypefaceStyle() == Typeface.BOLD) {
        ops.add(new SetSpanOperation(start, end, new StyleSpan(getTypefaceStyle())));
      }
    }

    // gradient
    if (getTextAttributes().mTextGradient != null) {
      ops.add(new SetSpanOperation(
          start, end, new LynxTextGradientSpan(getTextAttributes().mTextGradient)));
    }
  }

  private boolean isNeedSetLineHeightSpan() {
    return (!MeasureUtils.isUndefined(mTextAttributes.mLineHeight)
               // not set lineHeightSpan when using BoringLayout
               && !(isTextBoringLayoutEnabled() && isTextRefactorEnabled()
                   && getTextAttributes().getWhiteSpace() == WHITESPACE_NOWRAP
                   && !getTextAttributes().hasImageSpan()
                   && !getTextAttributes().hasInlineViewSpan()))
        // set lineHeightSpan for adjusting baseline
        || isEnableTextSingleLineVerticalAlignForSpan();
  }

  private boolean isSingLineAndOverflowClip() {
    return mTextAttributes.getTextOverflow() == TEXTOVERFLOW_CLIP
        && (mTextAttributes.getMaxLineCount() == 1
            || mTextAttributes.getWhiteSpace() == StyleConstants.WHITESPACE_NOWRAP);
  }

  private boolean isEnableTextSingleLineVerticalAlignForSpan() {
    // only support single line, not contains inline image and inline view
    return mTextAttributes.mTextSingleLineVerticalAlign != StyleConstants.VERTICAL_ALIGN_DEFAULT
        && !mTextAttributes.mHasInlineViewSpan && !mTextAttributes.mHasImageSpan;
  }

  public static class SetSpanOperation {
    protected int start, end;
    protected Object what;

    public SetSpanOperation(int start, int end, Object what) {
      this.start = start;
      this.end = end;
      this.what = what;
    }

    public void execute(SpannableStringBuilder sb) {
      // All spans will automatically extend to the right of the text, but not the
      // left - except
      // for spans that start at the beginning of the text.
      int spanFlags = Spannable.SPAN_EXCLUSIVE_INCLUSIVE;
      if (start == 0) {
        spanFlags = Spannable.SPAN_INCLUSIVE_INCLUSIVE;
      }
      // inline-image and inline-view
      if (what instanceof AbsInlineImageSpan || what instanceof NativeLayoutNodeSpan) {
        spanFlags = Spannable.SPAN_EXCLUSIVE_EXCLUSIVE;
      }
      sb.setSpan(what, start, end, spanFlags);
    }
  }

  private void setEnableFontScaling(boolean enableFontScaling) {
    mEnableFontScaling = enableFontScaling;
    setFontSize(mTextAttributes.mFontSize);
    for (int i = 0; i < getChildCount(); i++) {
      ShadowNode child = getChildAt(i);
      if (child instanceof BaseTextShadowNode) {
        ((BaseTextShadowNode) child).setEnableFontScaling(mEnableFontScaling);
      }
    }
  }

  @Override
  public void addChildAt(ShadowNode child, int i) {
    super.addChildAt(child, i);
    if (child instanceof BaseTextShadowNode) {
      ((BaseTextShadowNode) child).setEnableFontScaling(mEnableFontScaling);
    }
  }

  @LynxProp(name = PropsConstants.BITMAP_GRADIENT)
  public void setEnableBitmapGradient(boolean enable) {
    mEnableBitmapGradient = enable;
    if (mTextAttributes.mTextGradient != null) {
      mTextAttributes.mTextGradient.setEnableBitmapGradient(enable);
    }
  }

  @LynxProp(name = PropsConstants.TEXT_INDENT)
  public void setTextIndent(ReadableArray array) {
    if (array == null || array.size() != 2) {
      mTextAttributes.mTextIndent = null;
    } else {
      mTextAttributes.mTextIndent = new TextIndent(array);
    }
    markDirty();
  }

  // android-emoji-compat={true}
  @LynxProp(name = PropsConstants.ANDROID_EMOJI_COMPAT)
  public void setEnableEmojiCompat(boolean enable) {
    mEnableEmojiCompat = enable;
    // Lazy init emoji compat
    if (mEnableEmojiCompat && sSupportEmojiCompat && sEmojiProcess == null) {
      try {
        // We reflect emoji class here since Lynx doesn't provide emoji2 dependency. The users who
        // want to use this API should add androidx.emoji2 dependency in their project.
        Class<?> c = Class.forName("androidx.emoji2.text.EmojiCompat");
        Method method = c.getDeclaredMethod("get");
        sEmojiCompatInst = method.invoke(null);
        sEmojiProcess = c.getDeclaredMethod("process", CharSequence.class);
        // Set transparent background for emoji span
        Method setEmojiSpanIndicatorColor =
            c.getDeclaredMethod("setEmojiSpanIndicatorColor", int.class);
        setEmojiSpanIndicatorColor.invoke(sEmojiCompatInst, Color.TRANSPARENT);
      } catch (Exception e) {
        sSupportEmojiCompat = false;
        LLog.e(TAG, "enable emoji e: " + e);
      }
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.TEXT)
  public void setText(@Nullable Dynamic text) {
    mText = TextHelper.convertRawTextValue(text);

    markDirty();
  }
}
