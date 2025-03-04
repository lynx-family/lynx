// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_LTR;
import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_500;
import static com.lynx.tasm.behavior.StyleConstants.FONTWEIGHT_900;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_CENTER;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_LEFT;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_RIGHT;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_START;
import static com.lynx.tasm.behavior.StyleConstants.TEXTOVERFLOW_CLIP;

import android.graphics.Color;
import android.graphics.Typeface;
import android.text.Layout;
import android.text.TextDirectionHeuristic;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;
import androidx.annotation.ColorInt;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.ShadowData;
import com.lynx.tasm.behavior.ui.background.BackgroundGradientLayer;
import com.lynx.tasm.utils.PixelUtils;

@SuppressWarnings("unused")
public class TextAttributes {
  public static final int NOT_SET = -1;
  // CSS property
  public static final int TEXT_VERTICAL_ALIGN_TOP = 0;
  public static final int TEXT_VERTICAL_ALIGN_CENTER = 1;
  public static final int TEXT_VERTICAL_ALIGN_BOTTOM = 2;
  public static final int TEXT_VERTICAL_ALIGN_CUSTOM = 3;
  public static final String INLINE_IMAGE_PLACEHOLDER = "I";
  public static final String INLINE_BLOCK_PLACEHOLDER = "B";

  public static final int FIRST_CHAR_RTL_STATE_NONE_CHECK = 0;
  public static final int FIRST_CHAR_RTL_STATE_RTL = 1;
  public static final int FIRST_CHAR_RTL_STATE_LTR = 2;

  public int mMaxLineCount = NOT_SET;
  public int mMaxTextLength = NOT_SET;
  public Integer mFontColor = null;
  public int mTextAlign = TEXTALIGN_START;
  public int mDirection = DIRECTION_NORMAL;
  public int mTextVerticalAlign = NOT_SET;
  public int mFontWeight = Typeface.NORMAL;
  public int mFontStyle = Typeface.NORMAL;
  public int mWhiteSpace = StyleConstants.WHITESPACE_NORMAL;
  public int mTextOverflow = TEXTOVERFLOW_CLIP;
  public float mLineHeight = MeasureUtils.UNDEFINED;
  public float mLetterSpacing = MeasureUtils.UNDEFINED;
  public float mLineSpacing = 0;
  // mFontSize is set to 0 by default.
  // Should use setFontSize to set this variable before using it.
  public float mFontSize = 0;
  public TextIndent mTextIndent = null;
  public float mBaselineShift = 0.f;
  public boolean mHasImageSpan = false;
  public boolean mHasInlineViewSpan = false;
  public boolean mIsBoringSpan = false;
  // Determine whether contain system padding for font
  public boolean mIncludePadding = false;
  public String mFontFamily = null;
  private boolean mIsAutoFontSize = false;
  private float mAutoFontSizeMaxSize = 0.f;
  private float mAutoFontSizeMinSize = 0.f;
  private float mAutoFontSizeStepGranularity = 1.f;
  private float[] mAutoFontSizePresetSizes;
  public boolean mHasValidTypeface = false;
  private boolean mHyphen = false;

  public int mTextSingleLineVerticalAlign = StyleConstants.VERTICAL_ALIGN_DEFAULT;

  public int mFirstCharacterRTLState = FIRST_CHAR_RTL_STATE_NONE_CHECK;

  // text-shadow property
  public @Nullable ShadowData mTextShadow;

  // text-decoration property
  public @StyleConstants.TextDecoration int mTextDecoration;
  // text-decoration-style, 4 means solid
  public int mTextDecorationStyle = 4;
  // text-decoration-color, 0 is the default value
  public int mTextDecorationColor = 0;

  public @ColorInt int mTextStrokeColor = Color.TRANSPARENT;
  public float mTextStrokeWidth = 0;

  public BackgroundGradientLayer mTextGradient = null;

  public int getTypefaceStyle() {
    int style = Typeface.NORMAL;
    if (isFontWeightBOLD() && mFontStyle == Typeface.ITALIC) {
      style = Typeface.BOLD_ITALIC;
    } else if (isFontWeightBOLD()) {
      style = Typeface.BOLD;
    } else if (mFontStyle == Typeface.ITALIC) {
      style = mFontStyle;
    }
    return style;
  }

  private boolean isFontWeightBOLD() {
    return mFontWeight == Typeface.BOLD
        || (mFontWeight >= FONTWEIGHT_500 && mFontWeight <= FONTWEIGHT_900);
  }

  public TextAttributes copy() {
    TextAttributes attributes = new TextAttributes();
    attributes.mMaxLineCount = mMaxLineCount;
    attributes.mMaxTextLength = mMaxTextLength;
    attributes.mFontColor = mFontColor;
    attributes.mTextAlign = mTextAlign;
    attributes.mTextVerticalAlign = mTextVerticalAlign;
    attributes.mFontWeight = mFontWeight;
    attributes.mFontStyle = mFontStyle;
    attributes.mWhiteSpace = mWhiteSpace;
    attributes.mTextOverflow = mTextOverflow;
    attributes.mLineHeight = mLineHeight;
    attributes.mLetterSpacing = mLetterSpacing;
    attributes.mLineSpacing = mLineSpacing;
    attributes.mFontSize = mFontSize;
    attributes.mTextIndent = mTextIndent;
    attributes.mBaselineShift = mBaselineShift;
    attributes.mHasImageSpan = mHasImageSpan;
    attributes.mHasInlineViewSpan = mHasInlineViewSpan;
    attributes.mIsBoringSpan = mIsBoringSpan;
    attributes.mIncludePadding = mIncludePadding;
    attributes.mFontFamily = mFontFamily;
    attributes.mTextShadow = mTextShadow;
    attributes.mTextDecoration = mTextDecoration;
    attributes.mTextDecorationStyle = mTextDecorationStyle;
    attributes.mTextDecorationColor = mTextDecorationColor;
    attributes.mTextStrokeWidth = mTextStrokeWidth;
    attributes.mTextStrokeColor = mTextStrokeColor;
    attributes.mDirection = mDirection;
    attributes.mFirstCharacterRTLState = mFirstCharacterRTLState;
    attributes.mIsAutoFontSize = mIsAutoFontSize;
    attributes.mAutoFontSizeMinSize = mAutoFontSizeMinSize;
    attributes.mAutoFontSizeMaxSize = mAutoFontSizeMaxSize;
    attributes.mAutoFontSizeStepGranularity = mAutoFontSizeStepGranularity;
    attributes.mAutoFontSizePresetSizes = mAutoFontSizePresetSizes;
    attributes.mTextSingleLineVerticalAlign = mTextSingleLineVerticalAlign;
    attributes.mHasValidTypeface = mHasValidTypeface;
    attributes.mHyphen = mHyphen;
    return attributes;
  }

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof TextAttributes))
      return false;
    TextAttributes attr = (TextAttributes) o;
    return mMaxLineCount == attr.mMaxLineCount && mMaxTextLength == attr.mMaxTextLength
        && this.fontColorEquals(attr.mFontColor) && mTextAlign == attr.mTextAlign
        && mTextVerticalAlign == attr.mTextVerticalAlign && mFontWeight == attr.mFontWeight
        && mFontStyle == attr.mFontStyle && mWhiteSpace == attr.mWhiteSpace
        && mTextOverflow == attr.mTextOverflow && mLineHeight == attr.mLineHeight
        && mLetterSpacing == attr.mLetterSpacing && mLineSpacing == attr.mLineSpacing
        && mFontSize == attr.mFontSize && mTextIndent == attr.mTextIndent
        && mBaselineShift == attr.mBaselineShift && mHasImageSpan == attr.mHasImageSpan
        && mIsBoringSpan == attr.mIsBoringSpan && mHasInlineViewSpan == attr.mHasInlineViewSpan
        && mIncludePadding == attr.mIncludePadding
        && TextUtils.equals(mFontFamily, attr.mFontFamily) && mTextShadow == attr.mTextShadow
        && mTextDecoration == attr.mTextDecoration && mDirection == attr.mDirection
        && mTextDecorationColor == attr.mTextDecorationColor
        && mTextDecorationStyle == attr.mTextDecorationStyle
        && mTextStrokeColor == attr.mTextStrokeColor && mTextStrokeWidth == attr.mTextStrokeWidth
        && mFirstCharacterRTLState == attr.mFirstCharacterRTLState
        && mIsAutoFontSize == attr.mIsAutoFontSize
        && mAutoFontSizeMinSize == attr.mAutoFontSizeMinSize
        && mAutoFontSizeMaxSize == attr.mAutoFontSizeMaxSize
        && mAutoFontSizeStepGranularity == attr.mAutoFontSizeStepGranularity
        && mAutoFontSizePresetSizes == attr.mAutoFontSizePresetSizes
        && mTextSingleLineVerticalAlign == attr.mTextSingleLineVerticalAlign
        && mHasValidTypeface == attr.mHasValidTypeface && mHyphen == attr.mHyphen;
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = mMaxLineCount;
    result = result * prime + mMaxTextLength;
    result = result * prime + (mFontColor != null ? mFontColor : Color.BLACK);
    result = result * prime + mTextAlign;
    result = result * prime + mTextVerticalAlign;
    result = result * prime + mFontWeight;
    result = result * prime + mWhiteSpace;
    result = result * prime + mTextOverflow;
    result = result * prime + Float.floatToIntBits(mLineHeight);
    result = result * prime + Float.floatToIntBits(mLetterSpacing);
    result = result * prime + Float.floatToIntBits(mLineSpacing);
    result = result * prime + Float.floatToIntBits(mFontSize);
    result = result * prime + (mTextIndent == null ? 0 : mTextIndent.hashCode());
    result = result * prime + Float.floatToIntBits(mBaselineShift);
    result = result * prime + (mHasImageSpan ? 1 : 0);
    result = result * prime + (mIsBoringSpan ? 1 : 0);
    result = result * prime + (mHasInlineViewSpan ? 1 : 0);
    result = result * prime + (mIncludePadding ? 1 : 0);
    result = result * prime + (mFontFamily == null ? 0 : mFontFamily.hashCode());
    result = result * prime + (mTextShadow == null ? 0 : mTextShadow.hashCode());
    result = result * prime + mTextDecoration;
    result = result * prime + mTextDecorationStyle;
    result = result * prime + mTextDecorationColor;
    result = result * prime + Float.floatToIntBits(mTextStrokeWidth);
    result = result * prime + mTextStrokeColor;
    result = result * prime + mDirection;
    result = result * prime + (mTextGradient == null ? 0 : mTextGradient.hashCode());
    result = result * prime + mFirstCharacterRTLState;
    result = result * prime + (mIsAutoFontSize ? 1 : 0);
    result = result * prime + Float.floatToIntBits(mAutoFontSizeMinSize);
    result = result * prime + Float.floatToIntBits(mAutoFontSizeMaxSize);
    result = result * prime + Float.floatToIntBits(mAutoFontSizeStepGranularity);
    result = result * prime
        + (mAutoFontSizePresetSizes == null ? 0 : mAutoFontSizePresetSizes.hashCode());
    result = result * prime + mTextSingleLineVerticalAlign;
    result = result & prime + (mHasValidTypeface ? 1 : 0);
    result = result * prime + (mHyphen ? 1 : 0);
    return result;
  }

  public int getMaxLineCount() {
    return mMaxLineCount;
  }

  public void setMaxLineCount(int mMaxLineCount) {
    this.mMaxLineCount = mMaxLineCount;
  }

  public int getMaxTextLength() {
    return mMaxTextLength;
  }

  public void setMaxTextLength(int mMaxTextLength) {
    this.mMaxTextLength = mMaxTextLength;
  }

  public int getFontColor() {
    return mFontColor;
  }

  public void setFontColor(int mFontColor) {
    this.mFontColor = mFontColor;
  }

  public int getTextAlign() {
    return mTextAlign;
  }

  public Layout.Alignment getLayoutAlignment(boolean isRtl) {
    if (mTextAlign == TEXTALIGN_LEFT) {
      if (mDirection == DIRECTION_NORMAL) {
        return isRtl ? Layout.Alignment.ALIGN_OPPOSITE : Layout.Alignment.ALIGN_NORMAL;
      } else if (mDirection == DIRECTION_LTR) {
        return Layout.Alignment.ALIGN_NORMAL;
      } else {
        return Layout.Alignment.ALIGN_OPPOSITE;
      }
    } else if (mTextAlign == TEXTALIGN_RIGHT) {
      if (mDirection == DIRECTION_NORMAL) {
        return isRtl ? Layout.Alignment.ALIGN_NORMAL : Layout.Alignment.ALIGN_OPPOSITE;
      } else if (mDirection == DIRECTION_LTR) {
        return Layout.Alignment.ALIGN_OPPOSITE;
      } else {
        return Layout.Alignment.ALIGN_NORMAL;
      }
    } else if (mTextAlign == TEXTALIGN_CENTER) {
      return Layout.Alignment.ALIGN_CENTER;
    }

    return Layout.Alignment.ALIGN_NORMAL;
  }

  public Layout.Alignment getLayoutAlignment() {
    return getLayoutAlignment(false);
  }

  public void setTextAlign(int mTextAlign) {
    this.mTextAlign = mTextAlign;
  }

  public int getFontWeight() {
    return mFontWeight;
  }

  public void setFontWeight(int mFontWeight) {
    this.mFontWeight = mFontWeight;
  }

  public int getFontStyle() {
    return mFontStyle;
  }

  public void setFontStyle(int mFontStyle) {
    this.mFontStyle = mFontStyle;
  }

  public int getWhiteSpace() {
    return mWhiteSpace;
  }

  public void setWhiteSpace(int mWhiteSpace) {
    this.mWhiteSpace = mWhiteSpace;
  }

  public int getTextOverflow() {
    return mTextOverflow;
  }

  public void setTextOverflow(int mTextOverflow) {
    this.mTextOverflow = mTextOverflow;
  }

  public float getLineHeight() {
    return mLineHeight;
  }

  public void setLineHeight(float mLineHeight) {
    this.mLineHeight = mLineHeight;
  }

  public float getLetterSpacing() {
    return mLetterSpacing;
  }

  public void setLetterSpacing(float mLetterSpacing) {
    this.mLetterSpacing = mLetterSpacing;
  }

  public float getLineSpacing() {
    return mLineSpacing;
  }

  public void setLineSpacing(float mLineSpacing) {
    this.mLineSpacing = mLineSpacing;
  }

  public float getFontSize() {
    return mFontSize;
  }

  public void setFontSize(float mFontSize) {
    this.mFontSize = mFontSize;
  }

  public boolean hasImageSpan() {
    return mHasImageSpan;
  }

  public void setHasImageSpan(boolean mHasImageSpan) {
    this.mHasImageSpan = mHasImageSpan;
  }

  public boolean isBoringSpan() {
    return mIsBoringSpan;
  }

  public void setIsBoringSpan(boolean isBoringSpan) {
    this.mIsBoringSpan = isBoringSpan;
  }

  public boolean hasInlineViewSpan() {
    return mHasInlineViewSpan;
  }

  public void setHasInlineViewSpan(boolean hasInlineViewSpan) {
    this.mHasInlineViewSpan = hasInlineViewSpan;
  }

  public boolean isIncludePadding() {
    return mIncludePadding;
  }

  public void setIncludePadding(boolean mIncludePadding) {
    this.mIncludePadding = mIncludePadding;
  }

  public void setTextStrokeColor(int textStrokeColor) {
    mTextStrokeColor = textStrokeColor;
  }

  public int getTextStrokeColor() {
    return mTextStrokeColor;
  }

  public float getTextStrokeWidth() {
    return mTextStrokeWidth;
  }

  public void setTextStrokeWidth(float textStrokeWidth) {
    mTextStrokeWidth = textStrokeWidth;
  }

  public String getFontFamily() {
    return mFontFamily;
  }

  public void setFontFamily(String mFontFamily) {
    this.mFontFamily = mFontFamily;
  }

  public ShadowData getTextShadow() {
    if (mTextShadow == null) {
      mTextShadow = new ShadowData();
    }
    return mTextShadow;
  }

  public void setTextShadow(ShadowData mTextShadow) {
    this.mTextShadow = mTextShadow;
  }

  public void ensureTextShadow() {
    if (mTextShadow == null) {
      mTextShadow = new ShadowData();
    }
  }

  public TextDirectionHeuristic getDirectionHeuristic() {
    if (mDirection == DIRECTION_NORMAL) {
      return TextDirectionHeuristics.FIRSTSTRONG_LTR;
    } else if (mDirection == DIRECTION_LTR) {
      return TextDirectionHeuristics.LTR;
    } else {
      return TextDirectionHeuristics.RTL;
    }
  }

  private boolean fontColorEquals(@Nullable Integer color) {
    if (mFontColor != null && color != null) {
      return mFontColor.equals(color);
    } else if (mFontColor == null) {
      return color == null;
    } else {
      return false;
    }
  }

  public boolean getIsAutoFontSize() {
    return mIsAutoFontSize;
  }

  public void setAutoFontSize(ReadableArray autoFontSize) {
    if (autoFontSize == null || autoFontSize.size() != 4) {
      mIsAutoFontSize = false;
      return;
    }
    mIsAutoFontSize = autoFontSize.getBoolean(0);
    mAutoFontSizeMinSize = (float) autoFontSize.getDouble(1);
    mAutoFontSizeMaxSize = (float) autoFontSize.getDouble(2);
    mAutoFontSizeStepGranularity = (float) autoFontSize.getDouble(3);
  }

  public float getAutoFontSizeMaxSize() {
    return mAutoFontSizeMaxSize;
  }

  public float getAutoFontSizeMinSize() {
    return mAutoFontSizeMinSize;
  }

  public float getAutoFontSizeStepGranularity() {
    return mAutoFontSizeStepGranularity > 0.f ? mAutoFontSizeStepGranularity : 1.f;
  }

  public void setAutoFontSizePresetSizes(ReadableArray presetSizes) {
    if (presetSizes == null || presetSizes.size() == 0) {
      mAutoFontSizePresetSizes = null;
      return;
    }
    mAutoFontSizePresetSizes = new float[presetSizes.size()];
    for (int i = 0; i < presetSizes.size(); i++) {
      mAutoFontSizePresetSizes[i] = (float) presetSizes.getDouble(i);
    }
  }

  public float[] getAutoFontSizePresetSizes() {
    return mAutoFontSizePresetSizes;
  }

  public void setHasValidTypeface(boolean hasTypefaceUpdated) {
    mHasValidTypeface = hasTypefaceUpdated;
  }

  public boolean getHyphen() {
    return mHyphen;
  }

  public void setHyphen(boolean enable) {
    mHyphen = enable;
  }
}
